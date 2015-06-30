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


#ifndef SHTTL_BIT_H
#define SHTTL_BIT_H

#include <cassert>
#include <limits>
#include <iostream>
#include <iomanip>

#include "shttl_types.h"

namespace shttl
{

    static std::numeric_limits<u64> u64_limits;
    static const u64 u64_bits = 64; // for older compiler

    // signed/unsigned minimum/maximum values in N-bit sequence
    inline static u64 umax(const ssize_t bit_num) 
    {
        SHTTL_ASSERT(0 <= bit_num && bit_num <= u64_limits.digits);
        u64 bit = ((bit_num >> 6) & 1) ^ 1;
        return (bit << (bit_num & 63)) - 1ull;
    }

    inline static u64 umin(const ssize_t bit_num) 
    {
        SHTTL_ASSERT(0 <= bit_num && bit_num <= u64_limits.digits);
        return 0;
    }

    inline static s64 smax(const ssize_t bit_num) 
    {
        SHTTL_ASSERT(2 <= bit_num && bit_num <= u64_limits.digits);
        return static_cast<s64>(umax(bit_num - 1));
    }

    inline static s64 smin(const ssize_t bit_num) 
    {
        SHTTL_ASSERT(2 <= bit_num && bit_num <= u64_limits.digits);
        return static_cast<s64>(~0ull << (bit_num - 1));
    }


    // bit mask and bit test

    inline static u64 mask(const ssize_t bit_pos, const ssize_t bit_width = 1) 
    {
        const ssize_t p = bit_pos;
        const ssize_t w = bit_width;
        SHTTL_ASSERT(0 <= p     && p            <  u64_limits.digits);
        SHTTL_ASSERT(0 <=     w &&     w      <= u64_limits.digits);
        SHTTL_ASSERT(0 <= p + w && p + w - 1  <  u64_limits.digits);
        return umax(w) << p;
    }

    inline static bool test(const u64 value, const ssize_t bit_pos) 
    {
        return (value & mask(bit_pos)) != 0;
    }

    inline static u64 field(
        const u64 value, 
        const ssize_t bit_pos, 
        const ssize_t bit_width ) 
    {
        return (value >> bit_pos) & mask(0, bit_width);
    }

    inline static u64 set_bit(const u64 src, const ssize_t bit_pos) 
    {
        return src |  mask(bit_pos);
    }
    inline static u64 reset_bit(const u64 src, const ssize_t bit_pos) 
    {
        return src & ~mask(bit_pos);
    }

    inline static u64 deposit(
        const u64 value, 
        const ssize_t bit_pos, 
        const ssize_t bit_width, 
        const u64 r ) 
    {
        const u64 u =  value & ~mask(bit_pos, bit_width);
        const u64 s = (r &  mask(0, bit_width)) << bit_pos;
        return u | s;
    }


    // Sign Extension
    inline static s64 sign_ext(const u64 value, const ssize_t bit_num) 
    {
        SHTTL_ASSERT(1 < bit_num && bit_num <= u64_limits.digits);
        u64 sign_mask = (u64)1 << (bit_num - 1);
        u64 bit_mask  = sign_mask - 1;
        return (s64)( 
            (0ull - (value & sign_mask)) | (value & bit_mask));

    }

    // value を，bit_num ビットの値だと考えて count ビットだけ左ローテートする
    inline static u64 rotate_left(u64 value, ssize_t bit_num, int count)
    {
        value &= mask(0, bit_num);
        count %= bit_num;

        if (count == 0)
            return value;
        else
            return ((value << count) | (value >> (bit_num-count))) & mask(0, bit_num);
    }

    // value を，bit_num ビットの値だと考えて count ビットだけ右ローテートする
    inline static u64 rotate_right(u64 value, ssize_t bit_num, int count)
    {
        value &= mask(0, bit_num);
        count %= bit_num;

        if (count == 0)
            return value;
        else
            return ((value >> count) | (value << (bit_num-count))) & mask(0, bit_num);
    }

    // value を, bit_length 分XORで畳み込む
    inline static u64 xor_convolute(u64 value, int bit_length)
    {
        SHTTL_ASSERT(bit_length <= u64_limits.digits);

        u64 ret = 0;
        for(int bit_pos = 0; bit_pos < u64_limits.digits; bit_pos += bit_length){
            ret ^= field(value, bit_pos, bit_length);
        }
        return ret;
    }

}; // namespace shttl

#endif // SHTTL_BIT_H
