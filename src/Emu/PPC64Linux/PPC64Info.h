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


#ifndef __PPC64LINUX_PPC64INFO_H__
#define __PPC64LINUX_PPC64INFO_H__

#include "Interface/ISAInfo.h"

namespace Onikiri {
    namespace PPC64Linux {

        // PPC64のISA情報
        class PPC64Info : public ISAInfoIF
        {
        public:
            static const int InstructionWordBitSize = 32;
            static const int MaxSrcRegCount = 4;
            static const int MaxDstRegCount = 3;
            static const int MaxImmCount = 4;
            // Int : 32
            // FP  : 32
            // CR  : 8   (Condition Register)
            // SPR : 2   (Link Register, Count Register)
            // Flag : 2  (FPSCR, CF)
            // ADDR : 1
            static const int RegisterCount = 32 + 32 + 8 + 2 + 2 + 1;
            
            static const int REG_CR0     = 64;

            static const int REG_LINK    = 72;
            static const int REG_COUNT   = 73;
            //static const int REG_XER     = ;
            static const int REG_FPSCR   = 74;
            //static const int REG_SO      = ;
            static const int REG_CA      = 75;  // (carry flag) CA は独立したレジスタとして管理しておく
            static const int REG_ADDRESS = 76;

            // XERは使用されていないのでとりあえず未実装．ただし，CF (carry flag) は使用される

            static const int MAX_MEMORY_ACCESS_WIDTH = 8;


            virtual ISA_TYPE GetISAType();
            virtual int GetInstructionWordBitSize();
            virtual int GetRegisterWordBitSize();
            virtual int GetRegisterCount();
            virtual int GetAddressSpaceBitSize();
            virtual int GetMaxSrcRegCount();
            virtual int GetMaxDstRegCount();
            virtual int GetRegisterSegmentID(int regNum);
            virtual int GetRegisterSegmentCount();
            virtual int GetMaxOpInfoCountPerPC();
            virtual int GetMaxMemoryAccessByteSize();
            virtual bool IsLittleEndian();
        };


    } // namespace PPC64Linux
} // namespace Onikiri

#endif
