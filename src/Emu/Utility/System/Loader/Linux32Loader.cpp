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

#include "Emu/Utility/System/Loader/Linux32Loader.h"

#include "SysDeps/posix.h"
#include "Emu/Utility/System/Memory/MemorySystem.h"
#include "Emu/Utility/System/Memory/MemoryUtility.h"
#include "Emu/Utility/System/Loader/Elf32Reader.h"

using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::POSIX;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::EmulatorUtility::ELF;
using namespace Onikiri::EmulatorUtility::ELF32;

Linux32Loader::Linux32Loader(u16 machine)
    : m_imageBase(0), m_entryPoint(0), m_initialSp(0), m_machine(machine)
{

}

Linux32Loader::~Linux32Loader()
{
}

void Linux32Loader::LoadBinary(MemorySystem* memory, const String& command)
{
    Elf32Reader elfReader;

    try {
        elfReader.Open(command.c_str());

        if (elfReader.GetMachine() != m_machine)
            throw runtime_error((command + " : machine type does not match").c_str());

        m_bigEndian = elfReader.IsBigEndian();

        u64 imageBase = 0xffffffffffffffff; // An address where ELF file is loaded 
        u64 addrBase = 0xffffffffffffffff;  // The head of an allocated address space

        // Determine address/image bases
        for (int i = 0; i < elfReader.GetProgramHeaderCount(); i ++) {
            const Elf32Reader::Elf_Phdr &ph = elfReader.GetProgramHeader(i);
            if (ph.p_type == PT_LOAD) {
                if (addrBase > ph.p_vaddr) {
                    addrBase = ph.p_vaddr;
                }
                if (imageBase > ph.p_vaddr - ph.p_offset) {
                    imageBase = ph.p_vaddr - ph.p_offset;
                }
            }
        }

        u64 initialBrk = 0;
        for (int i = 0; i < elfReader.GetProgramHeaderCount(); i ++) {
            const Elf32Reader::Elf_Phdr &ph = elfReader.GetProgramHeader(i);
            VIRTUAL_MEMORY_ATTR_TYPE pageAttr =
                VIRTUAL_MEMORY_ATTR_READ | VIRTUAL_MEMORY_ATTR_WRITE;

            // Do not add a writable attribute to a code page.
            if(ph.p_flags & PF_X){
                pageAttr = VIRTUAL_MEMORY_ATTR_READ | VIRTUAL_MEMORY_ATTR_EXEC;
            }

            // セグメントのアドレス範囲に物理メモリを割り当て，セグメントを読み込む
            if (ph.p_type == PT_LOAD) {
                memory->AssignPhysicalMemory(ph.p_vaddr, ph.p_memsz, pageAttr);
                TargetBuffer buf(memory, ph.p_vaddr, static_cast<size_t>(ph.p_filesz));
                elfReader.ReadRange(static_cast<size_t>(ph.p_offset), (char*)buf.Get(), static_cast<size_t>(ph.p_filesz));

                initialBrk = max((u32)initialBrk, ph.p_vaddr+ph.p_memsz);

                if( memory->GetReservedAddressRange() >= ph.p_vaddr ){
                    THROW_RUNTIME_ERROR( "Reserved address region is too large.");
                }
            }

            // 実行属性のセグメントは1つと仮定
            if (ph.p_flags & PF_X) {
                m_codeRange.first = ph.p_vaddr;
                m_codeRange.second = static_cast<size_t>(ph.p_memsz);
            }
        }

        // Load header region if it is necessary
        if (addrBase > imageBase) {
            u64 size = addrBase - imageBase;
            memory->AssignPhysicalMemory(imageBase, size, VIRTUAL_MEMORY_ATTR_READ | VIRTUAL_MEMORY_ATTR_EXEC);
            TargetBuffer buf(memory, imageBase, static_cast<size_t>(size));
            elfReader.ReadRange(static_cast<size_t>(0), (char*)buf.Get(), static_cast<size_t>(size));
        }

        m_entryPoint = CalculateEntryPoint(memory, elfReader);
        m_imageBase = imageBase;

        memory->SetInitialBrk(initialBrk);

        m_elfProgramHeaderOffset = elfReader.GetProgramHeaderOffset();
        m_elfProgramHeaderCount = elfReader.GetProgramHeaderCount();

        // エントリポイント等の計算
        CalculateOthers(memory, elfReader);
    }
    catch (runtime_error& e) {
        THROW_RUNTIME_ERROR(e.what());
    }
}

void Linux32Loader::InitArgs(MemorySystem* memory, u64 stackHead, u64 stackSize, const String& command, const String& commandArgs)
{
    u32 sp = (u32)stackHead + (u32)stackSize;

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
    scoped_array<u32> target_argv ( new u32[argc+1] );
    for (int i = 0; i <= argc; i ++) {
        if (argv[i]) {
            size_t len = strlen(argv[i]);
            sp -= (u32)len + 1;
            memory->MemCopyToTarget(sp, argv[i], len+1);
            target_argv[i] = sp;
        }
        else {
            target_argv[i] = 0;
        }
    }
    sp -= sp % sizeof(u32);

    // <Linux>
    // stack : argc, argv[argc-1], NULL, environ[], NULL, auxv[], NULL

    // Auxiliary Vector の設定
    ELF32_AUXV auxv;

    sp -= sizeof(ELF32_AUXV);
    auxv.a_type = EndianHostToSpecified((u32)AT_NULL, m_bigEndian);
    auxv.a_un.a_val = 0;
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF32_AUXV));

    sp -= sizeof(ELF32_AUXV);
    auxv.a_type = EndianHostToSpecified((u32)AT_RANDOM, m_bigEndian);
    auxv.a_un.a_val = EndianHostToSpecified((u32)target_argv[0], m_bigEndian);
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF32_AUXV));

    sp -= sizeof(ELF32_AUXV);
    auxv.a_type = EndianHostToSpecified((u32)AT_SECURE, m_bigEndian);
    auxv.a_un.a_val = EndianHostToSpecified((u32)0, m_bigEndian);
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF32_AUXV));

    sp -= sizeof(ELF32_AUXV);
    auxv.a_type = EndianHostToSpecified((u32)AT_PAGESZ, m_bigEndian);
    auxv.a_un.a_val = EndianHostToSpecified((u32)memory->GetPageSize(), m_bigEndian);
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF32_AUXV));

    sp -= sizeof(ELF32_AUXV);
    auxv.a_type = EndianHostToSpecified((u32)AT_PHDR, m_bigEndian);
    auxv.a_un.a_val = EndianHostToSpecified((u32)(m_imageBase + m_elfProgramHeaderOffset), m_bigEndian);
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF32_AUXV));

    sp -= sizeof(ELF32_AUXV);
    auxv.a_type = EndianHostToSpecified((u32)AT_PHENT, m_bigEndian);
    auxv.a_un.a_val = EndianHostToSpecified((u32)sizeof(Elf32Reader::Elf_Phdr), m_bigEndian);
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF32_AUXV));

    sp -= sizeof(ELF32_AUXV);
    auxv.a_type = EndianHostToSpecified((u32)AT_PHNUM, m_bigEndian);
    auxv.a_un.a_val = EndianHostToSpecified((u32)m_elfProgramHeaderCount, m_bigEndian);
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF32_AUXV));

    sp -= sizeof(ELF32_AUXV);
    auxv.a_type = EndianHostToSpecified((u32)AT_ENTRY, m_bigEndian);
    auxv.a_un.a_val = EndianHostToSpecified((u32)m_codeRange.first, m_bigEndian);
    memory->MemCopyToTarget(sp, &auxv, sizeof(ELF32_AUXV));


    // environ
    sp -= sizeof(u32); // NULL
    // argv
    for (int i = 0; i <= argc; i ++) {
        sp -= sizeof(u32);
        WriteMemory( memory, sp, sizeof(u32), target_argv[argc-i]);
    }

    // argc
    sp -= sizeof(u32);
    WriteMemory( memory, sp, sizeof(u32), argc);

    m_initialSp = sp;
}

// Memory write
void Linux32Loader::WriteMemory( MemorySystem* memory, u64 address, int size, u64 value )
{
    EmuMemAccess access( address, size, value );
    memory->WriteMemory( &access );
}

u64 Linux32Loader::GetImageBase() const
{
    return m_imageBase;
}

u64 Linux32Loader::GetEntryPoint() const
{
    return m_entryPoint;
}

std::pair<u64, size_t> Linux32Loader::GetCodeRange() const
{
    return m_codeRange;
}

u64 Linux32Loader::GetInitialSp() const
{
    return m_initialSp;
}

// Get the stack tail (bottom)
u64 Linux32Loader::GetStackTail() const 
{
    return ADDR_STACK_TAIL;
}

bool Linux32Loader::IsBigEndian() const
{
    return m_bigEndian;
}

u64 Linux32Loader::CalculateEntryPoint(EmulatorUtility::MemorySystem* memory, const Elf32Reader& elfReader)
{
    return elfReader.GetEntryPoint();
}

// ElfReader を使って何かを計算する
void Linux32Loader::CalculateOthers(EmulatorUtility::MemorySystem* memory, const Elf32Reader& elfReader)
{
    // デフォルトでは何もしない
}
