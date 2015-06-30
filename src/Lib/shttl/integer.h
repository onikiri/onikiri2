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


#ifndef SHTTL_INTEGER_H
#define SHTTL_INTEGER_H

#include "shttl_types.h"

namespace shttl 
{

    //
    //  integer_base<T> 
    //

    template<class T>
    class integer_base {
    private:
        T _t;

    public:
        integer_base(T t = T()) : _t(t) {}
        operator T() const { return _t; }

    }; 


    //
    //  General Version
    //

    template<size_t N>
    struct integer : public integer_base<u64> {
        integer(u64 t = 0) : integer_base<u64>(t) {}
    };


    //
    //  Special Versions for 1 .. 8
    //

    template<>
    struct integer<1> : public integer_base<u8> {
        integer(u8 t = 0) : integer_base<u8>(t) {}
    };

    template<>
    struct integer<2> : public integer_base<u8> {
        integer(u8 t = 0) : integer_base<u8>(t) {}
    };

    template<>
    struct integer<3> : public integer_base<u8> {
        integer(u8 t = 0) : integer_base<u8>(t) {}
    };

    template<>
    struct integer<4> : public integer_base<u8> {
        integer(u8 t = 0) : integer_base<u8>(t) {}
    };

    template<>
    struct integer<5> : public integer_base<u8> {
        integer(u8 t = 0) : integer_base<u8>(t) {}
    };

    template<>
    struct integer<6> : public integer_base<u8> {
        integer(u8 t = 0) : integer_base<u8>(t) {}
    };

    template<>
    struct integer<7> : public integer_base<u8> {
        integer(u8 t = 0) : integer_base<u8>(t) {}
    };

    template<>
    struct integer<8> : public integer_base<u8> {
        integer(u8 t = 0) : integer_base<u8>(t) {}
    };


    //
    //  Special Versions for 9 .. 16
    //

    template<>
    struct integer<9> : public integer_base<u16> {
        integer(u16 t = 0) : integer_base<u16>(t) {}
    };

    template<>
    struct integer<10> : public integer_base<u16> {
        integer(u16 t = 0) : integer_base<u16>(t) {}
    };

    template<>
    struct integer<11> : public integer_base<u16> {
        integer(u16 t = 0) : integer_base<u16>(t) {}
    };

    template<>
    struct integer<12> : public integer_base<u16> {
        integer(u16 t = 0) : integer_base<u16>(t) {}
    };

    template<>
    struct integer<13> : public integer_base<u16> {
        integer(u16 t = 0) : integer_base<u16>(t) {}
    };

    template<>
    struct integer<14> : public integer_base<u16> {
        integer(u16 t = 0) : integer_base<u16>(t) {}
    };

    template<>
    struct integer<15> : public integer_base<u16> {
        integer(u16 t = 0) : integer_base<u16>(t) {}
    };

    template<>
    struct integer<16> : public integer_base<u16> {
        integer(u16 t = 0) : integer_base<u16>(t) {}
    };


    //
    //  Special Versions for 17 .. 32
    //

    template<>
    struct integer<17> : public integer_base<u32> {
        integer(u32 t = 0) : integer_base<u32>(t) {}
    };

    template<>
    struct integer<18> : public integer_base<u32> {
        integer(u32 t = 0) : integer_base<u32>(t) {}
    };

    template<>
    struct integer<19> : public integer_base<u32> {
        integer(u32 t = 0) : integer_base<u32>(t) {}
    };

    template<>
    struct integer<20> : public integer_base<u32> {
        integer(u32 t = 0) : integer_base<u32>(t) {}
    };

    template<>
    struct integer<21> : public integer_base<u32> {
        integer(u32 t = 0) : integer_base<u32>(t) {}
    };

    template<>
    struct integer<22> : public integer_base<u32> {
        integer(u32 t = 0) : integer_base<u32>(t) {}
    };

    template<>
    struct integer<23> : public integer_base<u32> {
        integer(u32 t = 0) : integer_base<u32>(t) {}
    };

    template<>
    struct integer<24> : public integer_base<u32> {
        integer(u32 t = 0) : integer_base<u32>(t) {}
    };

    template<>
    struct integer<25> : public integer_base<u32> {
        integer(u32 t = 0) : integer_base<u32>(t) {}
    };

    template<>
    struct integer<26> : public integer_base<u32> {
        integer(u32 t = 0) : integer_base<u32>(t) {}
    };

    template<>
    struct integer<27> : public integer_base<u32> {
        integer(u32 t = 0) : integer_base<u32>(t) {}
    };

    template<>
    struct integer<28> : public integer_base<u32> {
        integer(u32 t = 0) : integer_base<u32>(t) {}
    };

    template<>
    struct integer<29> : public integer_base<u32> {
        integer(u32 t = 0) : integer_base<u32>(t) {}
    };

    template<>
    struct integer<30> : public integer_base<u32> {
        integer(u32 t = 0) : integer_base<u32>(t) {}
    };

    template<>
    struct integer<31> : public integer_base<u32> {
        integer(u32 t = 0) : integer_base<u32>(t) {}
    };

    template<>
    struct integer<32> : public integer_base<u32> {
        integer(u32 t = 0) : integer_base<u32>(t) {}
    };

} // namespace shttl

#endif // SHTTL_INTEGER
