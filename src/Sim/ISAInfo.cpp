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

#include "Sim/ISAInfo.h"

using namespace Onikiri;

bool SimISAInfoDef::SimISAInfo_IW32_RW64_AS64::TestISAInfo(ISAInfoIF* info)
{
    int iw = info->GetInstructionWordBitSize();
    if(iw != 32){
        THROW_RUNTIME_ERROR(
            "An emulator of ISA with instruction"
            "word size(%d) is not supported.", iw );
    }

    int rw = info->GetRegisterWordBitSize();
    if(rw > 64){
        THROW_RUNTIME_ERROR(
            "An emulator of ISA with register"
            "word size(%d) is not supported.", rw );
    }

    int as = info->GetAddressSpaceBitSize();
    if(as > 64){
        THROW_RUNTIME_ERROR(
            "An emulator of ISA with address"
            "space size(%d) is not supported.", as );
    }

    int maxSN = info->GetMaxSrcRegCount();
    if(maxSN > MAX_SRC_REG_COUNT){
        THROW_RUNTIME_ERROR(
            "An emulator of ISA with max source register"
            "number(%d) is not supported.", maxSN );
    }

    int maxDN = info->GetMaxDstRegCount();
    if(maxDN > MAX_DST_REG_COUNT){
        THROW_RUNTIME_ERROR(
            "An emulator of ISA with max destination register"
            "number(%d) is not supported.", maxDN );
    }

    int regCnt = info->GetRegisterCount();
    if(regCnt > MAX_REG_COUNT){
        THROW_RUNTIME_ERROR(
            "An emulator of ISA with register"
            "number(%d) is not supported.", regCnt );
    }
    
    int maxOpInfo = info->GetMaxOpInfoCountPerPC();
    if(maxOpInfo > MAX_OP_INFO_COUNT_PER_PC){
        THROW_RUNTIME_ERROR(
            "An emulator of ISA with register"
            "number(%d) is not supported.", maxOpInfo );
    }

    int regSegmentCount = info->GetRegisterSegmentCount();
    if( regSegmentCount > MAX_REG_SEGMENT_COUNT ){
        THROW_RUNTIME_ERROR(
            "An emulator of ISA with register"
            "segments(%d) is not supported.", regSegmentCount );
    }

    return true;
}
