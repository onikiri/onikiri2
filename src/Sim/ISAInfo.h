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


#ifndef SIM_ISA_INFO_H
#define SIM_ISA_INFO_H

#include "Interface/ISAInfo.h"

//
// The following constants are used in Simulator section. 
// (ex. The number of max destinations
//
// Call 'TestISAInfo' and check whether the constants meet
// requirements by Emulator section.
// In Simulator section, You can get information of Emulator 
// section through 'ISAInfoIF'. These mechanisms are for
// avoiding defining fixed values in Interface section.
//

namespace Onikiri
{
    namespace SimISAInfoDef
    {
        //
        // Instruction width : 32bit
        // Register width    : 64bit
        // Address space     : 64bit
        //
        struct SimISAInfo_IW32_RW64_AS64
        {
            static const int INSTRUCTION_WORD_BIT_SIZE   = 32;
            static const int INSTRUCTION_WORD_BYTE_SIZE  = 4;
            static const int INSTRUCTION_WORD_BYTE_SHIFT = 2;

            static const int MAX_SRC_REG_COUNT = 4;
            static const int MAX_DST_REG_COUNT = 3;

            static const int MAX_REG_COUNT = 80;
            static const int MAX_REG_SEGMENT_COUNT = 6;
            static const int MAX_OP_INFO_COUNT_PER_PC = 4;

            static bool TestISAInfo(ISAInfoIF* info);
        };
    }

    typedef SimISAInfoDef::SimISAInfo_IW32_RW64_AS64 SimISAInfo;
};

#endif  // #ifdef SIM_ISA_INFO_H
