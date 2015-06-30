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


#ifndef __PPC64LINUX_TRAITS_H__
#define __PPC64LINUX_TRAITS_H__

#include "Emu/PPC64Linux/PPC64LinuxSyscallConv.h"
#include "Emu/PPC64Linux/PPC64LinuxLoader.h"
#include "Emu/PPC64Linux/PPC64Info.h"
#include "Emu/PPC64Linux/PPC64Converter.h"
#include "Emu/PPC64Linux/PPC64OpInfo.h"

namespace Onikiri {
    namespace PPC64Linux {

        struct PPC64LinuxTraits {
            typedef PPC64Info ISAInfoType;
            typedef PPC64OpInfo OpInfoType;
            typedef PPC64Converter ConverterType;
            typedef PPC64LinuxLoader LoaderType;
            typedef PPC64LinuxSyscallConv SyscallConvType;

            static const bool IsBigEndian = true;
        };

    } // namespace PPC64Linux
} // namespace Onikiri

#endif
