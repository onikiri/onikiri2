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

#ifndef UTILITY_STRING_H
#define UTILITY_STRING_H

#include <string>
#include <vector>
#include <stdarg.h>
#include <boost/tokenizer.hpp>

namespace Onikiri
{
    class String : public std::string
    {
        typedef std::string t_string;

    public:
        String();
        String(const t_string &str, size_type pos = 0, size_type n = npos);
        String(const char* str);
        String& format_arg(const char* fmt, va_list& arg);

        // 'printf' style format
        String& format(const char* fmt, ... );

        operator const char*() const {  return c_str(); };

        template<class T> String& operator=(const T& rhs)
        {
            assign(rhs);
            return *this;
        }

        template<class T> String operator+(const T& rhs)
        {
            String tmp(*this);
            tmp.append(rhs);
            return tmp;
        }

        String& operator=(const t_string& rhs)
        {
            assign(rhs);
            return *this;
        }

        String operator+(const t_string& rhs)
        {
            String tmp(*this);
            tmp.append(rhs);
            return tmp;
        }

        String& operator+=(const t_string& rhs)
        {
            append(rhs);
            return *this;
        }

        template<class T> String& operator+=(const T& rhs)
        {
            append(rhs);
            return *this;
        }

        String& operator+=(const char rhs)
        {
            append(&rhs, 1);
            return *this;
        }

        void find_and_replace( const String& from, const String& to )
        {
            while( true ){
                std::string::size_type i = find( from ); 

                if( i == std::string::npos ){
                    break;
                }

                std::string::replace( i, from.length(), to );
            }
        }

        // sepStr     : delimiter 
        // sepKeepStr : delimiter（分割後文字列にも残る）
        // ",/"なら，','と'/'をdelimiterとして文字列を分割
        std::vector<String> split(
            const char* delimiter, 
            const char* delimiterKeep = NULL) const;

        template <class T> T to()
        {
            return boost::lexical_cast<T>(*this);
        }
    };
}

#endif
