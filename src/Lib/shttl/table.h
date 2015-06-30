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


#ifndef SHTTL_TABLE_H
#define SHTTL_TABLE_H

#include <vector>
#include "shttl_types.h"
#include "replacement.h"
#include "lru.h"

namespace shttl
{
    //
    // A simple table with a replacement function.
    //
    template < typename type, typename replacer = lru<size_t> > 
    class table
    {
    public:
        typedef std::vector<type> array_type;
        typedef typename array_type::reference reference;
        typedef typename array_type::const_reference const_reference;

        table()
        {
        }

        table( const size_t size, const type& value = type() )
        {
            construct( size ,value );
        }

        void construct( const size_t size, const type& value = type() )
        {
            m_body.resize( size, value );
            m_replacement.construct( 1, size );
        }

        size_t replacement_target()
        {
            return m_replacement.target( 0 );
        }

        void touch( const size_t index )
        {
            m_replacement.touch( 0, index, index/*key_type*/ );
        }

        size_t size() const
        {
            return m_body.size();
        }

        reference operator[]( const size_t index )
        {
            return m_body[ index ];
        }

        const_reference operator[]( const size_t index ) const
        {
            return m_body[ index ];
        }

    protected:
        array_type  m_body;
        replacer m_replacement;

    };

}   // namespace shttl

#endif // #ifdef SHTTL_TABLE_H
