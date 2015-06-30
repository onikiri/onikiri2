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


#ifndef __EMULATORUTILITY_ELF64_H__
#define __EMULATORUTILITY_ELF64_H__

namespace Onikiri {
    namespace EmulatorUtility {

        namespace ELF64 {

            // ELF Header
            struct ELF64_HEADER
            {
                u8  e_ident[16];
                u16 e_type;
                u16 e_machine;
                u32 e_version;
                u64 e_entry;
                u64 e_phoff;
                u64 e_shoff;
                u32 e_flags;
                u16 e_ehsize;
                u16 e_phentsize;
                u16 e_phnum;
                u16 e_shentsize;
                u16 e_shnum;
                u16 e_shstrndx;
            };

            // ELF Section
            struct ELF64_SECTION
            {
                u32 sh_name;
                u32 sh_type;
                u64 sh_flags;
                u64 sh_addr;
                u64 sh_offset;
                u64 sh_size;
                u32 sh_link;
                u32 sh_info;
                u64 sh_addralign;
                u64 sh_entsize;
            };

            // ELF Program Header
            struct ELF64_PROGRAM
            {
                u32 p_type;
                u32 p_flags;
                u64 p_offset;
                u64 p_vaddr;
                u64 p_paddr;
                u64 p_filesz;
                u64 p_memsz;
                u64 p_align;
            };

            // auxiliary vector
            struct ELF64_AUXV
            {
                s64 a_type;
                union
                {
                    s64 a_val;

                    // 32bit/64bit 版でポインタが異なるので，使用しない．
                    //void *a_ptr;
                    //void (*a_fcn) (void);
                } a_un;
            };
        }   // namespace ELF64

        namespace ELF {
            // section type
            const u32 SHT_NOBITS = 8;

            // section flag
            const int SHF_WRITE     = 1 << 0;
            const int SHF_ALLOC     = 1 << 1;
            const int SHF_EXECINSTR = 1 << 2;

            // segment type
            const u32 PT_LOAD = 1;

            // segment flag
            const int PF_X = (1 << 0);
            const int PF_W = (1 << 1);
            const int PF_R = (1 << 2);

            // ident index
            const int EI_CLASS = 4;
            const int EI_DATA = 5;
            const int EI_VERSION = 6;

            // a_type
            const int AT_NULL = 0;
            const int AT_IGNORE = 1;
            const int AT_EXECFD = 2;
            const int AT_PHDR = 3;
            const int AT_PHENT = 4;
            const int AT_PHNUM = 5;
            const int AT_PAGESZ = 6;
            const int AT_BASE = 7;
            const int AT_FLAGS = 8;
            const int AT_ENTRY = 9;
            const int AT_NOTELF = 10;
            const int AT_UID = 11;
            const int AT_EUID = 12;
            const int AT_GID = 13;
            const int AT_EGID = 14;
            //const int AT_DCACHEBSIZE = 19;    // (PowerPC) Data Cache Block Size
        
        }   // namespace ELF

        using namespace ELF64;
        using namespace ELF;

    } // EmulatorUtility

    void EndianSpecifiedToHostInPlace(EmulatorUtility::ELF64_HEADER& h, bool bigEndian);
    void EndianSpecifiedToHostInPlace(EmulatorUtility::ELF64_SECTION& h, bool bigEndian);
    void EndianSpecifiedToHostInPlace(EmulatorUtility::ELF64_PROGRAM& h, bool bigEndian);
} // namespace Onikiri
#endif
