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


//
// Environmentally dependent debug element
//

#ifndef __ONIKIRI_DEBUG_H
#define __ONIKIRI_DEBUG_H

#include "SysDeps/host_type.h"


//
// File/Line/Function macros
//

#ifdef COMPILER_IS_MSVC

    #define ONIKIRI_DEBUG_FILE     __FILE__
    #define ONIKIRI_DEBUG_LINE     __LINE__
    #define ONIKIRI_DEBUG_FUNCTION __FUNCSIG__

#elif defined (COMPILER_IS_GCC) || defined(COMPILER_IS_CLANG)

    #define ONIKIRI_DEBUG_FILE     __FILE__
    #define ONIKIRI_DEBUG_LINE     __LINE__
    #define ONIKIRI_DEBUG_FUNCTION __PRETTY_FUNCTION__

#else

    #define ONIKIRI_DEBUG_FILE     __FILE__
    #define ONIKIRI_DEBUG_LINE     __LINE__
    #define ONIKIRI_DEBUG_FUNCTION __FUNCTION__

#endif

// 
// A macro to trigger a break manually for debug.
//

#ifdef COMPILER_IS_MSVC

#define ONIKIRI_BREAK() __debugbreak()

#elif defined(COMPILER_IS_GCC) || defined(COMPILER_IS_CLANG)

#define ONIKIRI_BREAK() __builtin_trap()

#else

// ONIKIRI_BREAK() is not supported in this compiler.
#define ONIKIRI_BREAK() 

#endif


#endif
