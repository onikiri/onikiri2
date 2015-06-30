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


#ifndef SHTTL_STD_HASHER_H
#define SHTTL_STD_HASHER_H

#include "bit.h"
#include "hasher.h"
#include <limits>
#include <stdexcept>

namespace shttl
{
    template<class T = size_t>
    class std_hasher : public hasher<T> {
    public:
        // --

        typedef shttl::hasher<T> base_type;
        typedef std_hasher<T>    this_type;

        typedef typename base_type::size_type  size_type;
        typedef T                          value_type;

        static const std::numeric_limits<value_type> value_info;
        

        // Accessors

        size_type  idx_bit()  const { return m_idx_bit;          }
        value_type idx_mask() const { return m_idx_mask;         }
        size_type  off_bit()  const { return m_off_bit;          }
        value_type off_mask() const { return m_off_mask;         }

        // size() returns maximum value of index()
        size_type  size()     const { return (size_type)(idx_mask() + 1); }


        // Constructor(s)
        explicit std_hasher(const size_type idx_bit_arg, const size_type off_bit_arg) :
            m_idx_bit ( idx_bit_arg ), 
            m_idx_mask( (value_type)mask(0, idx_bit_arg) ),
            m_off_bit ( off_bit_arg), 
            m_off_mask( (value_type)mask(0, off_bit_arg ) 
        ){
            if(( (size_type)value_info.digits < idx_bit() ) ||
               ( (size_type)value_info.digits < off_bit() ) ||
               ( (size_type)value_info.digits < idx_bit() + off_bit() )
            ){
                throw std::invalid_argument("std_hasher::std_hasher");
            }
        }

        ~std_hasher() 
        {
        }

        // Member Functions
        size_type index(const value_type& t) const 
        {
            return (size_type)((t >> off_bit()) & idx_mask());
        }

        value_type tag(const value_type& t) const
        {
            return t >> (off_bit()+idx_bit());
        }

        value_type rebuild(const value_type& tag, size_type index) const 
        {
            return static_cast<value_type>( ((tag << idx_bit()) | index) << off_bit() );
        }

        bool match(const value_type& lhs, const value_type& rhs) const 
        {
            return (lhs & ~off_mask()) == (rhs & ~off_mask());
        }

    protected:
        size_type   m_idx_bit;
        value_type  m_idx_mask;
        size_type   m_off_bit;
        value_type  m_off_mask;

    };

} // namespace shttl

#endif // SHTTL_STD_HASHER_H
