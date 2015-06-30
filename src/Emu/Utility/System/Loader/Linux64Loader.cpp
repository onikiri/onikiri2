// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Ryota Shioya.
// Copyright (c) 2005-2015 Masahiro Goshima.
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
// 
// 


#include <pch.h>

#include "Emu/Utility/System/Loader/Linux64Loader.h"

#include "SysDeps/posix.h"
#include "Emu/Utility/System/Memory/MemorySystem.h"
#include "Emu/Utility/System/Memory/MemoryUtility.h"
#include "Emu/Utility/System/Loader/ElfReader.h"

using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::POSIX;
using namespace Onikiri::EmulatorUtility;

Linux64Loader::Linux64Loader(u16 machine)
    : m_imageBase(0), m_entryPoint(0), m_initialSp(0), m_machine(machine)
{

}

Linux64Loader::~Linux64Loader()
{
}

void Linux64Loader::LoadBinary(MemorySystem* memory, const String& command)
{
    ElfReader elfReader;

    try {
        elfReader.Open(command.c_str());

        if (elfReader.GetMachine() != m_machine)
            throw runtime_error((command + " : machine type does not match").c_str());

        m_bigEndian = elfReader.IsBigEndian();

        u64 initialBrk = 0;
        for (int i = 0; i < elfReader.GetProgramHeaderCount(); i ++) {
            const ElfReader::Elf_Phdr &ph = elfReader.GetProgramHeader(i);

            VIRTUAL_MEMORY_ATTR_TYPE pageAttr =
                VIRTUAL_MEMORY_ATTR_READ | VIRTUAL_MEMORY_ATTR_WRITE;

            // Do not add a writable attribute to a code page.
            if(ph.p_flags & PF_X){
                pageAttr = VIRTUAL_MEMORY_ATTR_READ | VIRTUAL_MEMORY_ATTR_EXEC;
            }

            // セグメントのアドレス範囲に物理メモリを割り当て，セグメントを読み込む
            if (ph.p_type == PT_LOAD) {
                if (ph.p_offset == 0)
                    m_imageBase = ph.p_vaddr;
                memory->AssignPhysicalMemory(ph.p_vaddr, ph.p_memsz, pageAttr);
                TargetBuffer buf(memory, ph.p_vaddr, static_cast<size_t>(ph.p_filesz));
                elfReader.ReadRange(static_cast<size_t>(ph.p_offset), (char*)buf.Get(), static_cast<size_t>(ph.p_filesz));

                initialBrk = max(initialBrk, ph.p_vaddr+ph.p_memsz);

                if( memory->GetReservedAddressRange() >= ph.p_vaddr ){
                    THROW_RUNTIME_ERROR( "Reserved address region is too large.");
                }
            }
            //if (ph.p_flags & PF_W && writable_base == 0) {
            //}

            // 実行属性のセグメントは1つと仮定
            if (ph.p_flags & PF_X) {
                m_codeRange.first = ph.p_vaddr;
                m_codeRange.second = static_cast<size_t>(ph.p_memsz);
            }
        }
        ASSERT(m_imageBase != 0);

        memory->SetInitialBrk(initialBrk);

        m_elfProgramHeaderOffset = elfReader.GetProgramHeaderOffset();
        m_elfProgramHeaderCount = elfReader.GetProgramHeaderCount();

        // エントリポイント等の計算
        m_entryPoint = CalculateEntryPoint(memory, elfReader);
        CalculateOthers(memory, elfReader);
    }
    catch (runtime_error& e) {
        THROW_RUNTIME_ERROR(e.what());
    }
}

void Linux64Loader::InitArgs(MemorySystem* memory, u64 stackHead, u64 stackSize, const String& command, const String& commandArgs)
{
    u64 sp = stackHead+stackSize;

    // 引数の分解
    std::vector<String> stringArgs = commandArgs.split(" ");
    int argc = (int)stringArgs.size()+1;
    scoped_array<const char *> argv(new const char*[argc+1]);

    argv[0] = command.c_str();
    for (int i = 1; i < argc; i ++) {
        argv[i] = stringArgs[i-1].c_str();
    }
    argv[argc] = NULL;

    // スタック上に引数をコピー
    scoped_array<u64> target_argv ( new u64 [argc+1] );
    for (int i = 0; i <= argc; i ++) {
        if (argv[i]) {
            size_t len = strlen(argv[i]);
            sp -= len+1;
            memory->MemCopyToTarget(sp, argv[i], len+1);
            target_argv[i] = sp;
        }
        else {
            target_argv[i] = 0;
        }
    }
    sp -= sp % sizeof(u64);

    // <Linux>
    // stack : argc, argv[argc-1], NULL, environ[], NULL, auxv[], NULL

    // Auxiliary Vector の設定
    ELF64_AUXV auxv;

    const int uid = posix_getuid(), gid = posix_getgid(), euid = posix_geteuid(), egid = posix_getegid();

    sp -= sizeof(ELF64_AUXV);
    auxv.a_type = EndianHostToSpecified((u64)AT_NULL, m_bigEndian);
    auxv.a_un.a_val = 0;
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF64_AUXV));

    sp -= sizeof(ELF64_AUXV);
    auxv.a_type = EndianHostToSpecified((u64)AT_PHDR, m_bigEndian);
    auxv.a_un.a_val = EndianHostToSpecified((u64)(m_imageBase + m_elfProgramHeaderOffset), m_bigEndian);
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF64_AUXV));

    sp -= sizeof(ELF64_AUXV);
    auxv.a_type = EndianHostToSpecified((u64)AT_PHNUM, m_bigEndian);
    auxv.a_un.a_val = EndianHostToSpecified((u64)m_elfProgramHeaderCount, m_bigEndian);
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF64_AUXV));

    sp -= sizeof(ELF64_AUXV);
    auxv.a_type = EndianHostToSpecified((u64)AT_PHENT, m_bigEndian);
    auxv.a_un.a_val = EndianHostToSpecified((u64)sizeof(ElfReader::Elf_Phdr), m_bigEndian);
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF64_AUXV));

    sp -= sizeof(ELF64_AUXV);
    auxv.a_type = EndianHostToSpecified((u64)AT_PAGESZ, m_bigEndian);
    auxv.a_un.a_val = EndianHostToSpecified((u64)memory->GetPageSize(), m_bigEndian);
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF64_AUXV));

    sp -= sizeof(ELF64_AUXV);
    auxv.a_type = EndianHostToSpecified((u64)AT_BASE, m_bigEndian);
    auxv.a_un.a_val = 0;
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF64_AUXV));

    sp -= sizeof(ELF64_AUXV);
    auxv.a_type = EndianHostToSpecified((u64)AT_UID, m_bigEndian);
    auxv.a_un.a_val = EndianHostToSpecified((u64)uid, m_bigEndian);
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF64_AUXV));

    sp -= sizeof(ELF64_AUXV);
    auxv.a_type = EndianHostToSpecified((u64)AT_EUID, m_bigEndian);
    auxv.a_un.a_val = EndianHostToSpecified((u64)euid, m_bigEndian);
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF64_AUXV));

    sp -= sizeof(ELF64_AUXV);
    auxv.a_type = EndianHostToSpecified((u64)AT_GID, m_bigEndian);
    auxv.a_un.a_val = EndianHostToSpecified((u64)gid, m_bigEndian);
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF64_AUXV));

    sp -= sizeof(ELF64_AUXV);
    auxv.a_type = EndianHostToSpecified((u64)AT_EGID, m_bigEndian);
    auxv.a_un.a_val = EndianHostToSpecified((u64)egid, m_bigEndian);
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF64_AUXV));

    sp -= sizeof(u64); // NULL
    // environ
    sp -= sizeof(u64);
    WriteMemory( memory, sp, sizeof(u64), sp-8);    // argv[argc] (== NULL) を指すようにする

    sp -= sizeof(u64); // NULL
    // argv
    for (int i = 0; i <= argc; i ++) {
        sp -= sizeof(u64);
        WriteMemory( memory, sp, sizeof(u64), target_argv[argc-i]);
    }

    // argc
    sp -= sizeof(u64);
    WriteMemory( memory, sp, sizeof(u64), argc);

    m_initialSp = sp;
}

// Memory write
void Linux64Loader::WriteMemory( MemorySystem* memory, u64 address, int size, u64 value )
{
    EmuMemAccess access( address, size, value );
    memory->WriteMemory( &access );
}

u64 Linux64Loader::GetImageBase() const
{
    return m_imageBase;
}

u64 Linux64Loader::GetEntryPoint() const
{
    return m_entryPoint;
}

std::pair<u64, size_t> Linux64Loader::GetCodeRange() const
{
    return m_codeRange;
}

u64 Linux64Loader::GetInitialSp() const
{
    return m_initialSp;
}

bool Linux64Loader::IsBigEndian() const
{
    return m_bigEndian;
}

u64 Linux64Loader::CalculateEntryPoint(EmulatorUtility::MemorySystem* memory, const ElfReader& elfReader)
{
    return elfReader.GetEntryPoint();
}

// ElfReader を使って何かを計算する
void Linux64Loader::CalculateOthers(EmulatorUtility::MemorySystem* memory, const ElfReader& elfReader)
{
    // デフォルトでは何もしない
}
