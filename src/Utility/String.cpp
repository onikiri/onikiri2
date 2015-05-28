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
// Extended string
//

#include <pch.h>
#include "Utility/String.h"

using namespace Onikiri;

String::String()
{
}

String::String(const t_string &str, size_type pos, size_type n) : 
    t_string(str, pos, n)
{
};

String::String(const char* str) : 
    t_string(str)
{
};

String& String::format_arg(const char* fmt, va_list& arg)
{
    
    for(int size = 128;;size *= 2){
        char* buf = new char[size];
        
        va_list work_arg;
        va_copy(work_arg, arg);

        int writeSize = ::vsnprintf(buf, size, fmt, work_arg);
        bool success = (writeSize < size) && (writeSize != -1);

        va_end(work_arg);

        if(success)
            this->assign(buf);

        delete[] buf;

        if(success)
            break;
    }

    
    return *this;
}

// 'printf' style format
String& String::format(const char* fmt, ... )
{
    va_list arg;
    va_start(arg, fmt);
    format_arg(fmt, arg);
    va_end(arg);

    return *this;
}



// sepStr     : delimiter 
// sepKeepStr : delimiter（分割後文字列にも残る）
// ",/"なら，','と'/'をdelimiterとして文字列を分割
std::vector<String> String::split(
    const char* delimiter, 
    const char* delimiterKeep) const
{
    using namespace boost;
    typedef tokenizer< char_separator<char> > tokenizer;
    char_separator<char> sep( delimiter, delimiterKeep );
    tokenizer tok(*this, sep);

    std::vector<String> ret;
    for(tokenizer::iterator i = tok.begin(); i != tok.end(); ++i){
        ret.push_back( *i );
    }
    return ret;
}
