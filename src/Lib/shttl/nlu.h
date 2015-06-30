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


#ifndef SHTTL_NLU_H
#define SHTTL_NLU_H

#include "shttl_types.h"
#include "bit.h"
#include "replacement.h"

namespace shttl 
{
    template< typename key_type >
    class nlu : replacer< key_type >
    { 
    public:
        typedef typename replacer< key_type >::size_type size_type;


        nlu() :
            m_way_num(0),
            m_index_num(0)
        {
        }

        void construct( const size_type set_num, const size_type way_num )
        {
            if( std::numeric_limits<bitmap_type>().digits < (int)way_num ){
                throw std::invalid_argument( 
                    "shttl::nlu_array() \n Specified way number is too large." 
                );
            }

            m_way_num = way_num;
            m_index_num = set_num;
            m_bitmap.resize( m_index_num, 0 );
        }

        size_type size()
        {
            return m_index_num;
        }

        void touch(
            const size_type index,
            const size_type way,
            const key_type  key
        ){
            get_set( index ).touch( way );
        }

        size_type target(
            const size_type index
        ){
            return get_set( index ).target();
        }


    protected:

        size_type           m_way_num; 
        size_type           m_index_num; 

        typedef u32 bitmap_type;
        typedef std::vector<bitmap_type> bitmap_array_type;
        bitmap_array_type m_bitmap;

        class set
        {
        public:

            set( bitmap_array_type::iterator bitmap, size_t way_num ) : 
              m_bitmap( bitmap ),
              m_way_num( way_num )
            {
            }

            size_t target()
            {
                for(size_type i = 0; i < m_way_num; i++){
                    if(!(*m_bitmap & (1 << i)))
                        return i;
                }

                SHTTL_ASSERT(0);
                return 0;           
            }
    
            void touch(const size_t way)
            {
            #ifdef SHTTL_DEBUG
                if ( (size_type)m_way_num <= way ){
                    throw std::out_of_range("nlu::touch");
                }
            #endif

                *m_bitmap |= 1 << way;
                if( *m_bitmap == mask(0, m_way_num) )
                    *m_bitmap = 0;          
            }

        protected:
            bitmap_array_type::iterator m_bitmap;
            size_t m_way_num;

        };

        set get_set( const size_type index )
        {
            return set( m_bitmap.begin() + index, m_way_num );
        }

    };
} // namespace shttl

#endif // SHTTL_NLU_H

