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
#include "Emu/Utility/System/Loader/ELF32.h"
#include "SysDeps/Endian.h"

using namespace std;
using namespace Onikiri;

void Onikiri::EmulatorUtility::ELF32::EndianSpecifiedToHostInPlace(EmulatorUtility::ELF32::ELF32_HEADER& h, bool bigEndian)
{
    // e_ident は変換不要
    Onikiri::EndianSpecifiedToHostInPlace(h.e_type, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.e_machine, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.e_version, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.e_entry, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.e_phoff, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.e_shoff, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.e_flags, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.e_ehsize, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.e_phentsize, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.e_phnum, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.e_shentsize, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.e_shnum, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.e_shstrndx , bigEndian);
}

void Onikiri::EmulatorUtility::ELF32::EndianSpecifiedToHostInPlace(EmulatorUtility::ELF32::ELF32_SECTION& h, bool bigEndian)
{
    Onikiri::EndianSpecifiedToHostInPlace(h.sh_name, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.sh_type, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.sh_flags, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.sh_addr, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.sh_offset, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.sh_size, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.sh_link, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.sh_info, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.sh_addralign, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.sh_entsize, bigEndian);
}

void Onikiri::EmulatorUtility::ELF32::EndianSpecifiedToHostInPlace(EmulatorUtility::ELF32::ELF32_PROGRAM& h, bool bigEndian)
{
    Onikiri::EndianSpecifiedToHostInPlace(h.p_type, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.p_flags, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.p_offset, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.p_vaddr, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.p_paddr, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.p_filesz, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.p_memsz, bigEndian);
    Onikiri::EndianSpecifiedToHostInPlace(h.p_align, bigEndian);
}
