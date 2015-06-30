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
// Simple counter
//

#ifndef SHTTL_COUNTER_H
#define SHTTL_COUNTER_H

#include <stdexcept>

#include "bit.h"

namespace shttl
{

    //
    // On shttl::counter, value_body_type is a type of a value itself.
    // On shttl::counter_array, value_body_type is a reference and 
    // m_value referes a element of a large array in shttl::counter_array.
    //
    template < typename value_type = u8, typename value_body_type = value_type >
    class counter_base
    {

    public:
        explicit counter_base(
            value_body_type value,
            value_type initv = value_type(),
            value_type min  = 0,
            value_type max  = 3,
            value_type add = 1,
            value_type sub = 1,
            value_type threshold = 0
        ) :
            m_value( value ),
            m_initv( initv ),
            m_min  ( min   ),
            m_max  ( max   ),
            m_add  ( add   ),
            m_sub  ( sub   ),
            m_threshold( threshold )
        {
        /*  static_assert( 
                sizeof(int) >= sizeof(value_type), 
                "The size of value_type is too large." 
            );*/
        }

        value_type initv() const
        {
            return m_initv;
        }

        value_type min() const
        {
            return m_min;
        }

        value_type max() const
        {
            return m_max;
        }

        value_type threshold() const
        {
            return m_threshold;
        }

        // Conversion to value_type
        operator value_type() const 
        {
            return m_value; 
        }

        void set(       
            value_type initv = value_type(),
            value_type min  = 0,
            value_type max  = 3,
            value_type add = 1,
            value_type sub = 1,
            value_type threshold = 0
        )
        {
            m_initv = initv;
            m_min = min;
            m_max = max;
            m_add = add;
            m_sub = sub;
            m_threshold = threshold;
        }

        void reset()
        {
            m_value = m_initv;
        }

        value_type inc()
        {
            int value = m_value;
            value += m_add;
            if( value > m_max ){
                value = m_max;
            }
            m_value = (value_type)value;
            return m_value;
        }

        value_type dec()
        {
            int value = m_value;
            value -= m_sub;
            if( value < m_min ){
                value = m_min;
            }
            m_value = (value_type)value;
            return m_value;
        }

        // Returns whether a value is above a threshold.
        bool above_threshold() const
        {
            return m_value >= m_threshold;
        }

    protected:
        value_body_type m_value;
        value_type m_initv;
        value_type m_min;
        value_type m_max;
        value_type m_add;
        value_type m_sub;
        value_type m_threshold;

    };

    template < typename value_type = u8 >
    class counter : public counter_base< value_type, value_type >
    {

    public:
        counter(
            value_type initv = value_type(),
            value_type min = 0,
            value_type max = 3,
            value_type add = 1,
            value_type sub = 1,
            value_type threshold = 0
        ) : 
            base_type( initv, initv, min, max, add, sub, threshold )
        {
        }

    protected:
        typedef counter_base< value_type, value_type > base_type;

    };


} // namespace shttl

#endif // SHTTL_COUNTER_H
