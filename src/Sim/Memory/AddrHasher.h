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


#ifndef SIM_MEMORY_ADR_HASHER_H
#define SIM_MEMORY_ADR_HASHER_H

#include "Lib/shttl/hasher.h"
#include "Interface/Addr.h"

namespace Onikiri
{
    class AdrHasher : public shttl::hasher<Addr>
    {
    public:
        // --

        typedef shttl::hasher<Addr> base_type;
        typedef AdrHasher           this_type;

        typedef base_type::size_type size_type;
        typedef Addr value_type;

        static const std::numeric_limits<u64> value_info;

    private:
        size_type   m_idx_bit;
        u64         m_idx_mask;
        size_type   m_off_bit;
        u64         m_off_mask;

    public:

        // size() returns maximum value of index()
        size_type size() const 
        {
            return (size_type)(m_idx_mask + 1); 
        }

        // Constructor(s)
        explicit AdrHasher(
            const size_type idx_bit_arg, 
            const size_type off_bit_arg
        ) :
            m_idx_bit ( idx_bit_arg ), 
            m_idx_mask( shttl::mask(0, idx_bit_arg) ),
            m_off_bit ( off_bit_arg ), 
            m_off_mask( shttl::mask(0, off_bit_arg ) 
        ){
            if( ( (size_type)value_info.digits < m_idx_bit ) ||
                ( (size_type)value_info.digits < m_off_bit ) ||
                ( (size_type)value_info.digits < m_idx_bit + m_off_bit )
            ){
                throw std::invalid_argument("AdrHasher::AdrHasher");
            }
        }

        ~AdrHasher() 
        {
        }

        // Member Functions
        size_type index(const value_type& t) const 
        {
            return (size_type)((t.address >> m_off_bit) & m_idx_mask);
        }

        value_type tag(const value_type& t) const
        {
            value_type adr(t);
            adr.address >>= (m_off_bit+m_idx_bit);
            return adr;
        }

        value_type rebuild(const value_type& tag, size_type index) const 
        {
            value_type adr(tag);
            adr.address = ((tag.address << m_idx_bit) | index) << m_off_bit;
            return adr;
        }

        bool match(const value_type& lhs, const value_type& rhs) const 
        {
            return lhs == rhs;
        }

    };

} // namespace Onikiri

#endif // __ADR_HASHER_H
