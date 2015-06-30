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
#include "Emu/PPC64Linux/PPC64LinuxLoader.h"

#include "Emu/Utility/System/Memory/MemorySystem.h"
#include "Emu/Utility/System/Loader/ElfReader.h"
#include "Emu/Utility/System/Memory/MemoryUtility.h"

#include "Emu/PPC64Linux/PPC64Info.h"


using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::PPC64Linux;

namespace {
    const u16 MACHINE_PPC64 = 0x0015;
}

PPC64LinuxLoader::PPC64LinuxLoader()
    : Linux64Loader(MACHINE_PPC64)  // machine = PPC64
{
}

PPC64LinuxLoader::~PPC64LinuxLoader()
{
}

u64 PPC64LinuxLoader::GetInitialRegValue(int index) const
{
    const int STACK_POINTER_REGNUM = 1;
    const int TOC_POINTER_REGNUM = 2;
    const int TLS_POINTER_REGNUM = 13;

    switch (index) {
    case STACK_POINTER_REGNUM:
        return GetInitialSp();
    case TOC_POINTER_REGNUM:
        return m_startTocPointer;
    case PPC64Info::REG_FPSCR:
        return 0;
    case TLS_POINTER_REGNUM:
        // <TODO>
    default:
        return 0;
    }
}

u64 PPC64LinuxLoader::CalculateEntryPoint(EmulatorUtility::MemorySystem* memory, const ElfReader& elfReader)
{
    // ELFのエントリポイントには_start関数の関数デスクリプタが格納されており，offset 0に関数ポインタが格納されている
    //return memory->ReadMemory(elfReader.GetEntryPoint(), 8);
    EmuMemAccess access( elfReader.GetEntryPoint(), 8 );
    memory->ReadMemory( &access );
    return access.value;
}


void PPC64LinuxLoader::CalculateOthers(EmulatorUtility::MemorySystem* memory, const ElfReader& elfReader)
{
    // 関数デスクリプタのoffset 8がtoc
    //m_startTocPointer = memory->ReadMemory(elfReader.GetEntryPoint()+8, 8);

    EmuMemAccess access( elfReader.GetEntryPoint()+8, 8 );
    memory->ReadMemory( &access );
    m_startTocPointer = access.value;
}

// AT_DCACHEBSIZE: cache_line_size
// AT_SECURE: dl-support.c, enbl-secure.c

