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


#ifndef SHTTL_TYPES_H
#define SHTTL_TYPES_H


//
// basic data types
//
#ifdef _MSC_VER

    namespace shttl
    {
        typedef signed   __int64 s64;
        typedef unsigned __int64 u64;
        typedef signed   __int32 s32;
        typedef unsigned __int32 u32;
        typedef signed   __int16 s16;
        typedef unsigned __int16 u16;
        typedef signed   __int8  s8;
        typedef unsigned __int8  u8;

        typedef float  f32;
        typedef double f64;
    };
    
#else
 
    #include <inttypes.h>
    namespace shttl
    {
        typedef int64_t  s64;
        typedef uint64_t u64;
        typedef int32_t  s32;
        typedef uint32_t u32;
        typedef int16_t  s16;
        typedef uint16_t u16;
        typedef int8_t   s8;
        typedef uint8_t  u8;

        typedef float  f32;
        typedef double f64;
    };

#endif

namespace shttl
{
    typedef s64 ssize_t;
}

// --- debug macro ---
#ifndef SHTTL_DISABLE_DEBUG
#define SHTTL_DEBUG
#endif

#ifdef SHTTL_DEBUG

    #ifndef SHTTL_ASSERT
        #define SHTTL_ASSERT(x) assert(x)
    #endif

#else 

    #ifndef SHTTL_ASSERT
        #define SHTTL_ASSERT(x)
    #endif

#endif


#endif  // #ifndef SHTTL_TYPES_H

