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


#ifndef SHTTL_DOUBLE_HASHER_H
#define SHTTL_DOUBLE_HASHER_H

#include <utility>

#include "static_off_hasher.h"

namespace shttl 
{


    template<class T, ssize_t OFFSET = 0, class U = T >
    class double_hasher :
        public hasher< std::pair<T, U> >
    {
    public:
        typedef static_off_hasher<T, OFFSET>   base_type;
        typedef typename base_type::size_type  size_type;

    protected:
        base_type m_bash_hasher;

    public:

        // Types
        typedef double_hasher<T, OFFSET, U>    this_type;
        typedef std::pair<T, U>                value_type;


        size_type size() const  { return m_bash_hasher.size(); }


        // Constructors
        explicit double_hasher(const size_t i) :
            m_bash_hasher(i)
        { 
        }


        // Member Functions
        size_type index(const value_type& p) const 
        {
            return ((p.first >> m_bash_hasher. off_bit()) ^ p.second) & m_bash_hasher.idx_mask();
        }

        bool match(const value_type& lhs, const value_type& rhs) const 
        {
            return ((m_bash_hasher.match(lhs.first, rhs.first)) &&
                ((lhs.second & m_bash_hasher.idx_mask()) == (rhs.second & m_bash_hasher.idx_mask())));
        }

        value_type tag(const value_type& t) const
        {
            return 
                std::make_pair(
                    m_bash_hasher.tag(t.first),
                    t.second 
                );
        }

        value_type rebuild(const value_type& tag, size_type index) const 
        {
            return 
                std::make_pair(
                    m_bash_hasher.rebuild(tag.first, index),
                    tag.second 
                );
        }

    }; // class double_hasher

} // namespace shttl

#endif // SHTTL_DOUBLE_HASHER_H
