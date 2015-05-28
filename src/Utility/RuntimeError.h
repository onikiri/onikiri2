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
// Exception utility
// *** DO NOT USE THESE UTILITY MACROS IN CONSTRUCTOR ***
// These macros use exception mechanism and it can not work in constructor.
//

#ifndef UTILITY_RUNTIME_ERROR_H
#define UTILITY_RUNTIME_ERROR_H

#include "Utility/String.h"
#include "SysDeps/Debug.h"

namespace Onikiri
{
    void SetAssertNoThrow( bool noThrow );
    void SuppressWaning( bool suppress );
    bool IsInException();

    struct RuntimeErrorInfo
    {
        const char* file; 
        int line;
        const char* func;
        const char* name;
        const char* cond;
        RuntimeErrorInfo(){};
        RuntimeErrorInfo(
            const char* fileArg, 
            int lineArg,
            const char* funcArg,
            const char* nameArg,
            const char* condArg
        ) : 
            file( fileArg ),
            line( lineArg ),
            func( funcArg ),
            name( nameArg ),
            cond( condArg )
        {
        };
    };

    void RuntimeErrorFunction( const RuntimeErrorInfo& info, const char* fmt, ... );
    void RuntimeWarningFunction( const RuntimeErrorInfo& info, const char* fmt, ... );

    void AssertFunction( const RuntimeErrorInfo& info, bool assertCond, const char* fmt, ... );
    void AssertFunction( const RuntimeErrorInfo& info, bool assertCond );

    // Source file & function name
    static const char* Who()
    {
        return "Unknown";
    }
    #define ONIKIRI_DEBUG_NAME Who()

    #define DEBUG_WHERE_ARGS() \
        ONIKIRI_DEBUG_FILE , ONIKIRI_DEBUG_LINE, ONIKIRI_DEBUG_FUNCTION, ONIKIRI_DEBUG_NAME

    #if defined(COMPILER_IS_GCC) || defined(COMPILER_IS_CLANG)
        // Runtime exception
        #define THROW_RUNTIME_ERROR(...) \
            RuntimeErrorFunction(   RuntimeErrorInfo( DEBUG_WHERE_ARGS(), "" ), ## __VA_ARGS__ )

        // Runtime warning message
        #define RUNTIME_WARNING(...) \
            RuntimeWarningFunction( RuntimeErrorInfo( DEBUG_WHERE_ARGS(), "" ), ## __VA_ARGS__ )
    #else
        // Runtime exception
        #define THROW_RUNTIME_ERROR(...) \
            RuntimeErrorFunction(   RuntimeErrorInfo( DEBUG_WHERE_ARGS(), "" ), __VA_ARGS__ )

        // Runtime warning message
        #define RUNTIME_WARNING(...) \
            RuntimeWarningFunction( RuntimeErrorInfo( DEBUG_WHERE_ARGS(), "" ), __VA_ARGS__ )
    #endif

    // Assert
    #ifdef ONIKIRI_DEBUG
    #if defined(COMPILER_IS_GCC) || defined(COMPILER_IS_CLANG)
            // Assert
            #define ASSERT(cond, ...) \
                if(!(cond)){AssertFunction( RuntimeErrorInfo( DEBUG_WHERE_ARGS(), #cond ), false, ## __VA_ARGS__);}
        #else
            // C++0x supports a variable arguments length macro (__VA_ARGS__).
            // Assert
            #define ASSERT(cond, ...) \
                if(!(cond)){AssertFunction( RuntimeErrorInfo( DEBUG_WHERE_ARGS(), #cond ), false, __VA_ARGS__);}
        #endif
    #else
        #if defined(COMPILER_IS_MSVC) || defined(COMPILER_IS_GCC)
            #define ASSERT(...)
        #else
            inline void AssertDummy(bool, const char* str, ...){};
            inline void AssertDummy(bool){};
            #define ASSERT AssertDummy
        #endif
    #endif


} // namespace Onikiri


#endif
