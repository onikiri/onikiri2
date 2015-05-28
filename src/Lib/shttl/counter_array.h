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
// Array of counter
//

#ifndef SHTTL_COUNTER_ARRAY_H
#define SHTTL_COUNTER_ARRAY_H

#include <vector>
#include "counter.h"

namespace shttl 
{
    template < typename T = u8 >
    class counter_array 
    {
    public:

        typedef size_t              size_type;
        typedef T                   value_type;
        typedef counter_array<T>    this_type;
        typedef counter_base<T, T&> counter_type;

        // Accessors
        size_type  size()  const    { return m_array.size();    }
        value_type initv() const    { return m_initv;           }
        value_type min() const      { return m_min;             }
        value_type max() const      { return m_max;             }
        value_type threshold() const{ return m_threshold;       }

        // Constructor
        explicit counter_array(
            size_type  size  = 0,
            value_type initv = value_type(),
            value_type min  = 0,
            value_type max  = 3,
            value_type add = 1,
            value_type sub = 1,
            value_type threshold = 0
        )
        {
            construct( size, initv, min, max, add, sub, threshold );
        }

        void construct(
            size_type  size,
            value_type initv = value_type(),
            value_type min  = 0,
            value_type max  = 3,
            value_type add = 1,
            value_type sub = 1,
            value_type threshold = 0
        ){
            m_array.resize( size, initv );
            m_initv = initv;
            m_min   = min;
            m_max   = max;
            m_add   = add;
            m_sub   = sub;
            m_threshold = threshold;
        }

        ~counter_array()
        {
        }


        // Operator
        counter_type operator[](const size_type s)
        {
            return at( s );
        }

        counter_type at(const size_type s)
        {
            return counter_base<T, T&>(
                m_array[s],
                m_initv,
                m_min,
                m_max,
                m_add,
                m_sub,
                m_threshold
            );
        }

    protected:
        std::vector< value_type > m_array;
        
        value_type m_initv;
        value_type m_min;
        value_type m_max;
        value_type m_add;
        value_type m_sub;
        value_type m_threshold;

    }; // class counter_array

} // namespace shttl

#endif // SHTTL_COUNTER_ARRAY_H
