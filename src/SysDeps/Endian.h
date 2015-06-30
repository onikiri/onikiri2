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


#ifndef __SYSDEPS_ENDIAN_H__
#define __SYSDEPS_ENDIAN_H__

#include "SysDeps/host_type.h"

namespace Onikiri {

    inline u8 ConvertEndian(u8 value)
    {
        return value;
    }

    // バイトオーダー変換に環境依存の高速な実装が可能であれば使用する
#if defined(COMPILER_IS_MSVC)
    inline u16 ConvertEndian(u16 value)
    {
        return _byteswap_ushort(value);
    }
    inline u32 ConvertEndian(u32 value)
    {
        return _byteswap_ulong(value);
    }
    inline u64 ConvertEndian(u64 value)
    {
        return _byteswap_uint64(value);
    }

#define CONVERTENDIAN16_DEFINED
#define CONVERTENDIAN32_DEFINED
#define CONVERTENDIAN64_DEFINED

#elif defined(COMPILER_IS_GCC) || defined(COMPILER_IS_CLANG)
#if defined(HOST_IS_X86_64) || defined(HOST_IS_X86)
    inline u16 ConvertEndian(u16 value)
    {
        __asm__("rorw $8, %0" : "=r" (value) : "0" (value));
        return value;
    }

    inline u32 ConvertEndian(u32 value)
    {
        __asm__("bswap %0" : "=r" (value) : "0" (value));
        return value;
    }

#define CONVERTENDIAN16_DEFINED
#define CONVERTENDIAN32_DEFINED

#if defined(HOST_IS_X86_64)
    inline u64 ConvertEndian(u64 value)
    {
        __asm__("bswap %0" : "=r" (value) : "0" (value));
        return value;
    }
#define CONVERTENDIAN64_DEFINED
#endif // #if defined(HOST_IS_X86_64)

#endif // #if defined(HOST_IS_X86_64) || defined(HOST_IS_X86)
#endif // #elif defined(COMPILER_IS_GCC)



    // 環境依存の実装ができなければ汎用の実装を使用する
#ifndef CONVERTENDIAN16_DEFINED
    inline u16 ConvertEndian(u16 value)
    {
        return
            ((value & 0x00ff) << 8)
            | (value >> 8);
    }
#else
#   undef CONVERTENDIAN16_DEFINED
#endif // #ifndef CONVERTENDIAN16_DEFINED

#ifndef CONVERTENDIAN32_DEFINED
    inline u32 ConvertEndian(u32 value)
    {
        return
            ((u32)ConvertEndian((u16)value) << 16)
            | (u32)ConvertEndian((u16)(value >> 16));
    }
#else
#   undef CONVERTENDIAN32_DEFINED
#endif // #ifndef CONVERTENDIAN32_DEFINED

#ifndef CONVERTENDIAN64_DEFINED
    inline u64 ConvertEndian(u64 value)
    {
        return
            ((u64)ConvertEndian((u32)value) << 32)
            | (u64)ConvertEndian((u32)(value >> 32));
    }
#else
#   undef CONVERTENDIAN64_DEFINED
#endif // #ifndef CONVERTENDIAN64_DEFINED

    // signed → unsigned の変換時，自動型変換ではあいまいさが生じるので
    inline s8 ConvertEndian(s8 value)
    {
        return (s8)ConvertEndian((u8)value);
    }
    inline s16 ConvertEndian(s16 value)
    {
        return (s16)ConvertEndian((u16)value);
    }
    inline s32 ConvertEndian(s32 value)
    {
        return (s32)ConvertEndian((u32)value);
    }
    inline s64 ConvertEndian(s64 value)
    {
        return (s64)ConvertEndian((u64)value);
    }

    // size_t は環境によってサイズが異なるので，size_t のエンディアン変換を行ってはならない
    // 環境によって結果が異なってしまう

#if defined(HOST_IS_LITTLE_ENDIAN)
    template <typename T>
    inline T EndianHostToLittle(T value)
    {
        return value;
    }
    template <typename T>
    inline T EndianHostToBig(T value)
    {
        return ConvertEndian(value);
    }

#elif defined(HOST_IS_BIG_ENDIAN)
    template <typename T>
    inline T EndianHostToLittle(T value)
    {
        return ConvertEndian(value);
    }
    template <typename T>
    inline T EndianHostToBig(T value)
    {
        return value;
    }
#else
#   error "host endian is not specified"
#endif

    // HostToLittle と LittleToHost，HostToBig と BigToHost は，実際には必ず同じことをするわけだが，名前のために一応定義しておく
    template <typename T>
    inline T EndianLittleToHost(T value)
    {
        return EndianHostToLittle(value);
    }
    template <typename T>
    inline T EndianBigToHost(T value)
    {
        return EndianHostToBig(value);
    }

    // ターゲットのエンディアンを引数で指定するバージョン
    template <typename T>
    inline T EndianSpecifiedToHost(T value, bool bigEndian)
    {
        if (bigEndian)
            return EndianBigToHost(value);
        else
            return EndianLittleToHost(value);
    }
    template <typename T>
    inline T EndianHostToSpecified(T value, bool bigEndian)
    {
        return EndianSpecifiedToHost(value, bigEndian);
    }

    // 引数を参照で受けて変更するバージョン
    template <typename T>
    inline void EndianSpecifiedToHostInPlace(T &value, bool bigEndian) 
    {
        value = EndianSpecifiedToHost(value, bigEndian);
    }
    template <typename T>
    inline void EndianHostToSpecifiedInPlace(T &value, bool bigEndian)
    {
        value = EndianHostToSpecified(value, bigEndian);
    }

} // namespace Onikiri {

#endif
