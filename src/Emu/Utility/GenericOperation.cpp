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

namespace {

struct u128 {
    u64 high_part;
    u64 low_part;
};

//
// Calculate the lower 128 bit of a 128 bit x 128 bit product.
// In general, when a K-bit integer is multiplied by a K-bit integer,
// the lower K bits of the result will be the same whether the input is
// a signed or unsigned representation.
// This function calculates it for the case where K = 128.
// Since this algorithm works independently of the sign of the inputs,
// this function can be used to implement any of
// MulHhigh64SS/MulHigh64SU/MulHigh64UU function family.
// We don't use an implementation using signed multiplication
// (it may be slight efficient) because it is complex.
//
u128 UInt128Mul(u128 lhs, u128 rhs) noexcept {
    // Convert the inputs to base 2^32 notation.
    // This is to keep partial products less than 2^64
    // (they can be calculated with u64 without error).
    // lhs == lhs_array[3]*2^96 + lhs_array[2]*2^64
    //          + lhs_array[1]*2^32 + lhs_array[0]*2^0
    const u64 lhs_array[4] = {
        lhs.low_part & 0xffffffff,  // 2^0 term
        lhs.low_part >> 32,         // 2^32 term
        lhs.high_part & 0xffffffff, // 2^64 term
        lhs.high_part >> 32         // 2^96 term
    };
    const u64 rhs_array[4] = {
        rhs.low_part & 0xffffffff,  // 2^0 term
        rhs.low_part >> 32,         // 2^32 term
        rhs.high_part & 0xffffffff, // 2^64 term
        rhs.high_part >> 32         // 2^96 term
    };

    // result_array[4..8] are written but not used; they will be optimized out.
    u64 result_array[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    // Calculate partial products (the standard long multiplication algorithm)
    for( size_t i = 0; i < 4; ++i ) {
        for( size_t j = 0; j < 4; ++j ) {
            const u64 partial_product = lhs_array[i] * rhs_array[j];
            const size_t k = i + j;
            result_array[k+1] += partial_product >> 32;
            result_array[k]   += partial_product & 0xffffffff;
        }
    }

    // Normalize (carry-propagate addition)
    for( size_t k = 0; k < 8; ++k ) {
        result_array[k+1] += result_array[k] >> 32;
        result_array[k]   &= 0xffffffff;
    }

    u128 result;
    result.high_part = result_array[3]<<32 | result_array[2];
    result.low_part  = result_array[1]<<32 | result_array[0];

    return result;
}

u128 zext_u64_to_u128(u64 x) noexcept {
    u128 result;
    result.high_part = 0;
    result.low_part = x;
    return result;
}

u128 sext_s64_to_u128(s64 x) noexcept {
    u128 result;
    result.high_part = x < 0 ? 0xffffffffffffffff : 0;
    result.low_part = static_cast<u64>(x);
    return result;
}

s64 bit_cast_u64_to_s64(u64 x) noexcept {
    s64 result;
    memcpy(&result, &x, sizeof(result));
    return result;
}

u64 MulHigh64UU(u64 lhs, u64 rhs) noexcept {
    return UInt128Mul(zext_u64_to_u128(lhs), zext_u64_to_u128(rhs)).high_part;
}

s64 MulHigh64SU(s64 lhs, u64 rhs) noexcept {
    u128 whole_result = UInt128Mul(sext_s64_to_u128(lhs), zext_u64_to_u128(rhs));
    // Bitcast is needed to avoid signed integer overflow (undefined behavior).
    return bit_cast_u64_to_s64(whole_result.high_part);
}

s64 MulHigh64SS(s64 lhs, s64 rhs) noexcept {
    u128 whole_result = UInt128Mul(sext_s64_to_u128(lhs), sext_s64_to_u128(rhs));
    // Bitcast is needed to avoid signed integer overflow (undefined behavior).
    return bit_cast_u64_to_s64(whole_result.high_part);
}

} // namespace (anonymous)

//
// high-order 64 bits of the 128-bit product of unsigned lhs and rhs
//

u64 Onikiri::EmulatorUtility::Operation::UnsignedMulHigh64(u64 lhs, u64 rhs)
{
    return MulHigh64UU(lhs, rhs);
}

s64 Onikiri::EmulatorUtility::Operation::SignedMulHigh64(s64 lhs, s64 rhs)
{
    return MulHigh64SS(lhs, rhs);
}

s64 Onikiri::EmulatorUtility::Operation::SignedUnsignedMulHigh64(s64 lhs, u64 rhs)
{
    return MulHigh64SU(lhs, rhs);
}
