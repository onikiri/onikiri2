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
// Debug utility
//

#include <pch.h>
#include "Utility/RuntimeError.h"

using namespace std;

namespace Onikiri
{

    static bool g_inException = false;
    static bool g_suppressWarning = false;

    static String DebugWhere( const RuntimeErrorInfo& info ){
        return String().format(
            "%s(%d):\n  Method: %s\n  Object: %s \n",
            info.file,
            info.line, 
            info.func, 
            info.name
        );
    }

    static bool g_assertNoThrow = false;

    void SetAssertNoThrow( bool noThrow )
    {
        g_assertNoThrow = noThrow;
    }

    void SuppressWaning( bool suppress )
    {
        g_suppressWarning = suppress;
    }

    bool IsInException()
    {
        return g_inException;
    }



    void RuntimeErrorFunction( const RuntimeErrorInfo& info, const char* fmt, ...)
    {
        String str;

        va_list arg;
        va_start(arg, fmt);
        str.format_arg(fmt, arg);
        va_end(arg);

        std::runtime_error error("");
        if( String(info.file) == "" ){
            error = std::runtime_error(
                str + "\n\n"
            );
        }
        else{
            error = std::runtime_error(
                "\n" + DebugWhere( info ) + "  Message: " + str + "\n"
            );
        }

        if(g_inException){
            //printf( "%s\n",  error.what() );
            return;
        }
        g_inException = true;
        throw error;
    }

    //
    // Runtime warning
    //
    void RuntimeWarningFunction( const RuntimeErrorInfo& info, const char* fmt, ...)
    {
        String str;

        va_list arg;
        va_start(arg, fmt);
        str.format_arg(fmt, arg);
        va_end(arg);


        String msg;
        if( String(info.file) == "" ){
            msg = str;
        }
        else{
            msg = 
                DebugWhere( info ) + 
                "Warning:\n" +
                str;
        }

        if( !g_suppressWarning ){
            printf( "%s\n\n", msg.c_str() );
        }
    }

    //
    // Assert
    //

    void AssertFunction( const RuntimeErrorInfo& info, bool assertCond, const char* fmt, ... )
    {
        if(assertCond)
            return;
        if(g_inException)
            return;
        g_inException = true;

        if(g_assertNoThrow){
            assert(0);
        }
        else{
            String str;

            va_list arg;
            va_start(arg, fmt);
            str.format_arg(fmt, arg);
            va_end(arg);

            if( String(info.file) == "" ){
                throw std::runtime_error(
                    str + "\n"
                    );
            }
            else{
                throw std::runtime_error(
                    DebugWhere( info ) + 
                    "Assertion failed.\n" +
                    "Condition : " + info.cond + "\n"
                    + str + "\n"
                    );
            }
        }
    }
    
    void AssertFunction( const RuntimeErrorInfo& info, bool assertCond )
    {
        if(assertCond)
            return;
        if(g_inException)
            return;
        g_inException = true;

        if(g_assertNoThrow){
            assert(0);
        }
        else{
            throw std::runtime_error(
                DebugWhere( info ) +
                "Assertion failed.\n" +
                "Condition : " + info.cond + "\n"
                );
        }
    }
}

