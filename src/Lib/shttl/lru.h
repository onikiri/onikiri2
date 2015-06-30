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


#ifndef SHTTL_LRU_H
#define SHTTL_LRU_H

#include <limits>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <algorithm>

#include "shttl_types.h"
#include "bit.h"
#include "replacement.h"


namespace shttl 
{
    //
    // --- LRU replacer (time stamp based) ---
    //
    template< typename key_type >
    class lru_time : public replacer< key_type >
    {

    public:
        typedef typename replacer< key_type >::size_type size_type;

        void construct( const size_type set_num , const size_type way_num )
        {
            m_lru.resize( set_num );
            for( size_t i = 0; i < m_lru.size(); i++ ){
                m_lru[i].construct( way_num );
            }
        }

        size_type size()
        {
            return m_lru.size();
        }

        void touch(
            const size_type index,
            const size_type way,
            const key_type  key
        ){
            m_lru[ index ].touch( way );
        }

        size_type target(
            const size_type index
        ){
            return m_lru[ index ].target();
        }

    protected:
        class set
        {
        public:

            // Types
            typedef u8 counter_type;
            typedef size_t size_type;

            static const std::numeric_limits<counter_type>  counter_info;
            static const std::numeric_limits<size_type>     size_info;

            // Accessors(s)
            size_type size() const { return m_way_num; }


            // Constructors
            set( const size_type size ) :
            m_way_num(size), 
                m_stamp( new counter_type[size] )
            {
                reset();
            }

            set() : 
            m_way_num(0), 
                m_stamp(NULL)
            {
                m_mru  = 0;
                m_tick = 0;
            }

            void reset()
            {
                for (size_type w = 0; w < m_way_num; ++w)
                    m_stamp[w] = (counter_type)w;

                m_mru = m_way_num - 1;
                m_tick = (counter_type)m_way_num;
            }

            ~set()
            {
                if(m_stamp)
                    delete[] m_stamp;
            }

            void construct( const size_type way_num )
            {
                if(m_stamp)
                    delete[] m_stamp;

                m_way_num  = way_num;
                m_stamp = new counter_type[way_num];
                reset();
            }

            void touch( const size_type i )
            {
                if(size() <= i)
                    throw std::out_of_range("set::touch");

                if (i == m_mru) return;    // nothing to do

                if (m_tick == counter_info.max())
                    rewind();

                m_stamp[i] = m_tick++;      // main work of touch()
                m_mru = i;
            }

            size_type target()
            {                           // find i that m_stamp[i] is minimum
                size_type i   =        0;
                size_type min = m_stamp[0];
                for (size_type j = 1; j < size(); ++j) {
                    if (m_stamp[j] < min) {
                        i   =        j;
                        min = m_stamp[j];
                    }
                }
                return i;
            }


        protected:

            // Data Members
            size_type      m_way_num; // size

            counter_type*  m_stamp; // time stamp
            counter_type   m_tick;  // tick
            size_type      m_mru;   // index to most recently used entry

            void rewind()
            {
                std::vector< std::pair<counter_type, size_type> > v(size());

                for (size_type i = 0; i < size(); ++i)
                    v[i] = std::pair<counter_type, size_type>(m_stamp[i], i);

                sort(v.begin(), v.end());

                for (size_type i = 0; i < size(); ++i)
                    m_stamp[v[i].second] = (counter_type)i;

                SHTTL_ASSERT(m_stamp[m_mru] == size() - 1);
                m_tick = (counter_type)size();
            }

        }; // class set

        typedef std::vector<set> lru_array;
        typedef typename lru_array::reference reference;

        lru_array m_lru;
    };



    //
    // --- LRU replacer (order based) ---
    //

    template< typename key_type, typename order_type = u8 >
    class lru_order : public replacer< key_type >
    {
    public:
        typedef typename replacer< key_type >::size_type size_type;
        typedef typename std::vector<order_type>         order_array_type;
        typedef typename order_array_type::iterator      iterator;

        lru_order()
        {
            m_way_num = 0;
            m_set_num = 0;
        }

        void construct( const size_type set_num, const size_type way_num )
        {
            m_way_num = way_num;
            m_set_num = set_num;
            
            size_t size = set_num * way_num;

            if( std::numeric_limits<order_type>().max() <= way_num ){
                throw std::invalid_argument( 
                    "shttl::lru_order() \n THe specified way number is too large." 
                );
            }

            m_order.resize( size );

            for( size_t i = 0; i < set_num; i++ ){
                reset_set( i );
            }

        }

        size_type size()
        {
            return m_set_num;
        }

        size_type target( const size_type index )
        {
            return get_set( index )[0];
        }

        void touch( const size_type index, const size_type way, const key_type key )
        {
            iterator set = get_set( index );
            //         LRU             MRU
            // initial : 0 1 2 3 4 5 6 7
            // touch 0 : 1 2 3 4 5 6 7 0
            // touch 3 : 0 1 2 4 5 6 7 3
            bool sride = false;
            for( size_t w = 0; w < m_way_num; w++ ){
                if( sride ){
                    set[ w-1 ] = set[ w ];
                }
                else{
                    if( set[ w ] == way ){
                        sride = true;
                    }
                }
            }
            set[ m_way_num - 1 ] = (order_type)way;

        }

    protected:

        size_t           m_way_num; 
        size_t           m_set_num; 
        order_array_type m_order;

        iterator get_set( const size_type index )
        {
            size_t offset = index * m_way_num;
            return m_order.begin() + offset;
        }

        void reset_set( size_type index )
        {
            iterator set = get_set( index );
            for( size_t i = 0; i < m_way_num; i++ ){
                set[i] = (order_type)i; // compatible original lru
            }
        }


    };


    //
    // --- LRU replacer (linked list based) ---
    // 'lru_list' can touch()/target() in O(1) time.
    // ('lru_order' can do them in O(way_num) time, but it consumes
    // only half as much memory as 'lru_list'.
    //
    // LRU cannot be implemented by an usual linked list (ex. std::list),
    // because a replacer cannot know which entry should be touched.  
    //
    // This implementation uses a linked list that is constructed in a fixed size array.
    // On touch(), a touched entry is moved to the last of a list.
    // Thus, the first entry of a list is always LRU.
    // Way numbers corresponds to entries in a list respectively and 
    // which entry should be touched can be determined easily.
    //
    template< typename key_type, typename pointer_type = u8 >
    class lru_list : public replacer< key_type >
    {
    public:
        typedef typename replacer< key_type >::size_type size_type;

        struct element_type
        {
            pointer_type prev;
            pointer_type next;
        };

        typedef typename std::vector<element_type>  array_type;
        typedef typename array_type::iterator       iterator;

        lru_list()
        {
            m_way_num = 0;
            m_set_num = 0;
            m_pitch   = 0;
        }

        void construct( const size_type set_num, const size_type way_num )
        {
            m_way_num = way_num;
            m_set_num = set_num;
            m_pitch   = way_num + 1;
            size_t size = set_num * m_pitch;

            if( std::numeric_limits<pointer_type>().max() <= m_pitch ){
                throw std::invalid_argument( 
                    "shttl::lru_list() \n The specified way number is too large." 
                );
            }

            m_array.resize( size );

            for( size_t i = 0; i < set_num; i++ ){
                reset_set( i );
            }

        }

        size_type size()
        {
            return m_set_num;
        }

        size_type target( const size_type index )
        {
            return get_footer( index )->next;
        }

        void touch( const size_type index, const size_type way, const key_type key )
        {
            iterator set = get_set( index );
            iterator cur = set + way;
            
            // Erase 'cur' from list.
            erase( set, cur );    

            // Insert 'cur' before a footer.
            insert( 
                set, 
                set + m_way_num,  // This is a footer
                cur
            );

            //           Array&Pointer Image             List Image
            //           way-0 way-1 way-2 way-3 footer
            // initial : [4,1] [0,2] [1,3] [2,4] [3,0] / w0 -> w1 -> w2 -> w3 -> footer -> w0(LRU)
            // touch 0 : [3,4] [4,2] [1,3] [2,0] [0,1] / w1 -> w2 -> w3 -> w0 -> footer -> w1(LRU)
            // touch 3 : [2,3] [4,2] [1,0] [0,4] [3,1] / w1 -> w2 -> w0 -> w3 -> footer -> w1(LRU)
            //                                      ^
            //                                     LRU

        }

    protected:

        size_t     m_way_num; 
        size_t     m_pitch; 
        size_t     m_set_num; 
        array_type m_array;

        iterator get_set( const size_type index )
        {
            size_t offset = index * m_pitch;

            // LRU information of each cache set consists of a list with 'm_way_num + 1' ways.
            // (m_pitch = m_way_num + 1
            // The last entry in each list is a footer pointing an LRU entry.
            //
            // An example of 4-way array:
            // set-0: | way-0 | way-1 | way-2 | way-3 | footer |
            // set-1: | way-0 | way-1 | way-2 | way-3 | footer |
            // set-2: ...
            
            return m_array.begin() + offset;
        }


        iterator get_footer( const size_type index )
        {
            return get_set( index ) + m_way_num;
        }


        void reset_set( size_type index )
        {
            //      LRU <--------------------> MRU
            //       0        1        2        3    m_way_num
            // |-> way-0 -> way-1 -> way-2 -> way-3 -> footer ->|
            // ^                                                |
            // ^------------------------------------------------|

            iterator set = get_set( index );

            for( size_t i = 0; i < m_way_num + 1; i++ ){
                ( set + i )->prev = (pointer_type)(i - 1);
                ( set + i )->next = (pointer_type)(i + 1);
            }

            (set + 0)->prev = (pointer_type)m_way_num;
            (set + m_way_num)->next = (pointer_type)0;
        }

        // Erase 'cur' from a list of 'set'.
        void erase( iterator set, iterator cur )
        {
            iterator prev = set + cur->prev;
            iterator next = set + cur->next;
            prev->next = cur->next;
            next->prev = cur->prev;
        }

        // Insert 'cur' before 'pos'.
        void insert( iterator set, iterator pos, iterator cur )
        {
            iterator prev = set + pos->prev;
            cur->prev  = pos->prev;
            cur->next  = (pointer_type)(pos - set);
            prev->next = (pointer_type)(cur - set);
            pos->prev  = (pointer_type)(cur - set);
        }

    };



    // Use order based implementation.
    template< typename key_type >
    class lru : public lru_list< key_type >
    {
    };

} // namespace shttl

#endif // SHTTL_LRU_H
