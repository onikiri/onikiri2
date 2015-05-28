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
// 'stdarg.h' header for compatibility.
// va_copy macro is not defined in some environment.
//

#ifndef SYSDEPS_STDARG_H
#define SYSDEPS_STDARG_H

#include "SysDeps/host_type.h"


#include <stdarg.h>

#ifdef COMPILER_IS_MSVC
    #if _MSC_VER < 1800 
        #define va_copy(x, y) ((x) = (y))
    #endif
#elif defined(COMPILER_IS_GCC) || defined(COMPILER_IS_CLANG)
    #include <sys/types.h>
#else
    #define va_copy(x, y) ((x) = (y))
#endif

#endif
