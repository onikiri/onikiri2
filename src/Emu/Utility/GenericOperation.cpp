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
#include "Emu/Utility/GenericOperation.h"

using namespace std;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::EmulatorUtility::Operation;

//
// high-order 64 bits of the 128-bit product of unsigned lhs and rhs
//
u64 Onikiri::EmulatorUtility::Operation::UnsignedMulHigh64(u64 lhs, u64 rhs)
{
    // split to high and low part
    u64 lhs_h = lhs >> 32;
    u64 lhs_l = lhs & 0xffffffff;
    u64 rhs_h = rhs >> 32;
    u64 rhs_l = rhs & 0xffffffff;

    u64 result = lhs_h * rhs_h;

    u64 lhs_l_rhs_h = lhs_l * rhs_h;
    u64 lhs_h_rhs_l = lhs_h * rhs_l;

    u64 lhs_l_rhs_l = lhs_l * rhs_l;

    // Add to 'result' high-order 64 bits of '(lhs_l_rhs_h<<32) + (lhs_r_rhs_l<<32) + lhs_l_rhs_l'
    //
    // Now, low-order 32 bits of lhs_l_rhs_l does not affect the result (since it generates no carry),
    // so we can simply take high-order 32 bits of
    // 'lhs_l_rhs_h&0x00000000ffffffff + lhs_h_rhs_l&0x00000000ffffffff + lhs_l_rhs_l>>32'.
    // and high-order 64 bits of
    // '(lhs_l_rhs_h&0xffffffff00000000 + lhs_h_rhs_l&0xffffffff00000000) << 32'

    result += (lhs_l_rhs_h>>32) + (lhs_h_rhs_l>>32);
    result += ((lhs_l_rhs_h&0xffffffff) + (lhs_h_rhs_l&0xffffffff) + (lhs_l_rhs_l>>32))>>32;

    return result;
}

s64 Onikiri::EmulatorUtility::Operation::SignedMulHigh64(s64 lhs, s64 rhs)
{
    // 分かりやすい方法として，乗数・被乗数をどちらも正にして乗算を行い，最後に符号を調節する
    s64 resultSign = 1;
    if (lhs < 0) {
        lhs = -lhs;
        resultSign *= -1;
    }
    if (rhs < 0) {
        rhs = -rhs;
        resultSign *= -1;
    }
    s64 result = (s64)UnsignedMulHigh64((u64)lhs, (u64)rhs);
    return result*resultSign;
}

