// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Masahiro Goshima.
// Copyright (c) 2005-2017 Ryota Shioya.
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


#ifndef EMU_UTILITY_SYSTEM_LOADER_ELF32_H
#define EMU_UTILITY_SYSTEM_LOADER_ELF32_H

#include "Emu/Utility/System/Loader/ELF64.h"

namespace Onikiri {
    namespace EmulatorUtility {

        namespace ELF32 {

            // ELF Header
            struct ELF32_HEADER
            {
                u8  e_ident[16];
                u16 e_type;
                u16 e_machine;
                u32 e_version;
                u32 e_entry;
                u32 e_phoff;
                u32 e_shoff;
                u32 e_flags;
                u16 e_ehsize;
                u16 e_phentsize;
                u16 e_phnum;
                u16 e_shentsize;
                u16 e_shnum;
                u16 e_shstrndx;
            };

            // ELF Section (Elf32_Shdr)
            struct ELF32_SECTION
            {
                u32 sh_name;
                u32 sh_type;
                u32 sh_flags;
                u32 sh_addr;
                u32 sh_offset;
                u32 sh_size;
                u32 sh_link;
                u32 sh_info;
                u32 sh_addralign;
                u32 sh_entsize;
            };

            // ELF Program Header (Elf32_Phdr)
            // The field order of this structure is different from the 64bit version
            struct ELF32_PROGRAM
            {
                u32 p_type;
                u32 p_offset;
                u32 p_vaddr;
                u32 p_paddr;
                u32 p_filesz;
                u32 p_memsz;
                u32 p_flags;
                u32 p_align;
            };

            // auxiliary vector
            struct ELF32_AUXV
            {
                s32 a_type;
                union
                {
                    s32 a_val;

                    // 32bit/64bit 版でポインタが異なるので，使用しない．
                    //void *a_ptr;
                    //void (*a_fcn) (void);
                } a_un;
            };

            void EndianSpecifiedToHostInPlace(ELF32_HEADER& h, bool bigEndian);
            void EndianSpecifiedToHostInPlace(ELF32_SECTION& h, bool bigEndian);
            void EndianSpecifiedToHostInPlace(ELF32_PROGRAM& h, bool bigEndian);
        }   // namespace ELF32

    } // EmulatorUtility

} // namespace Onikiri
#endif
