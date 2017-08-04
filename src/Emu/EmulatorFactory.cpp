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


#include <pch.h>
#include "Emu/EmulatorFactory.h"
#include "Emu/AlphaLinux/AlphaLinuxEmulator.h"
#include "Emu/PPC64Linux/PPC64LinuxEmulator.h"
#include "Emu/RISCV32Linux/RISCV32LinuxEmulator.h"

using namespace Onikiri;

EmulatorFactory::EmulatorFactory()
{
}

EmulatorFactory::~EmulatorFactory()
{
}


EmulatorIF* EmulatorFactory::Create(const String& systemName, SystemIF* simSystem)
{
    if (systemName == "AlphaLinux") {
        return new AlphaLinux::AlphaLinuxEmulator( simSystem );
    }
    else if (systemName == "PPC64Linux") {
        return new PPC64Linux::PPC64LinuxEmulator(simSystem);
    }
    else if (systemName == "RISCV32Linux") {
        return new RISCV32Linux::RISCV32LinuxEmulator(simSystem);
    }

    THROW_RUNTIME_ERROR(
        "Unknown system name specified.\n"
        "This parameter must be one of the following strings : \n"
        "[AlphaLinux,PPC64Linux]"
    );

    return 0;
}

