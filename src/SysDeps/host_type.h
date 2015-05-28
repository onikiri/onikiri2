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


#ifndef __HOST_TYPE_H__
#define __HOST_TYPE_H__

// ä»à’ìIÇ…îªíË
// ñ{ìñÇÕconfigureçsÇ§

#if defined( __CYGWIN__ )
#   define HOST_IS_CYGWIN
#elif defined( _WIN32 )
#   define HOST_IS_WINDOWS
#else
#   define HOST_IS_LINUX
#endif

#if defined( HOST_IS_WINDOWS ) || defined(HOST_IS_CYGWIN) 

// _WIN32_WINNT should be defined for boost::asio
#define _WIN32_WINNT 0x0501

#endif

#if defined( _MSC_VER )
#   define COMPILER_IS_MSVC
#   if _MSC_VER < 1800 
#       error "Visual C++ 2013 or later is required"
#   endif
#elif defined( __clang__ )
#   define COMPILER_IS_CLANG
#elif defined( __GNUC__ )
#   define COMPILER_IS_GCC
#else
#   define COMPILER_IS_UNKNOWN
#endif

#if defined(COMPILER_IS_MSVC)
#   define HOST_IS_LITTLE_ENDIAN
#   ifdef _WIN64
#       define HOST_IS_X86_64
#   else
#       define HOST_IS_X86
#   endif
#elif defined(COMPILER_IS_GCC) || defined(COMPILER_IS_CLANG)
#   if defined(__x86_64__)
#       define HOST_IS_LITTLE_ENDIAN
#       define HOST_IS_X86_64
#   elif defined(__i386__) 
#       define HOST_IS_LITTLE_ENDIAN
#       define HOST_IS_X86
#   elif defined(__powerpc__) 
#       define HOST_IS_BIG_ENDIAN
#       define HOST_IS_POWERPC
#   else
#       error "unknown host architecture"
#   endif
#else
#   error "unknown compiler"
#endif

//#define HOST_IS_BIG_ENDIAN

#endif
