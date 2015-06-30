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


#ifndef SHTTL_SIMPLE_HASHER_H
#define SHTTL_SIMPLE_HASHER_H

#include <limits>
#include <stdexcept>

#include "hasher.h"
#include "bit.h"

namespace shttl 
{

    template<class T = int>
    class simple_hasher : public hasher<T> 
    {
    public:

        // Types
        typedef simple_hasher<T>    this_type;
        typedef hasher<T>           base_type;

        typedef typename base_type::size_type  size_type;
        typedef T                              value_type;

        static std::numeric_limits<value_type> value_info;

        // Accessors
        size_type size() const  { return m_idx_mask + 1; }

        // Constructors
        explicit simple_hasher(const size_type s) :
            m_idx_bit ( s ), 
            m_idx_mask( (size_type)mask(0, s) )
        {
            if( s <= 0 || (u64)value_info.max() < (u64)s ){
                throw std::invalid_argument("simple_hasher::simple_hasher");
            }
        } 

        // Member Functions
        size_type index(const T& t) const
        {  
            return static_cast<size_type>(t) & m_idx_mask;
        }

        // tag (‚Ó‚Â‚¤ãˆÊbit)
        T tag(const T& key) const
        {
            return key >> m_idx_bit;
        }
        
        // index‚Ætag‚©‚çkey‚ð•œŒ³‚·‚é
        T rebuild(const T& tag, size_type index) const
        {
            return (T)((tag << m_idx_bit) | index);
        }

        bool match( const T& lhs, const T& rhs ) const
        {
        #ifdef SHTTL_DEBUG
            if( size() < (size_type)lhs || size() < (size_type)rhs ){
                throw std::range_error("simple_hasher::match");
            }
        #endif
            return lhs == rhs;
        }

    protected:

        size_type  m_idx_bit;
        size_type  m_idx_mask;

    };

} // namespace shttl

#endif // SHTTL_SIMPLE_HASHER_H
