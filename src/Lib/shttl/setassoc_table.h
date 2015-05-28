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
// Generic set-associative table class.
//
// Usage:
//  shttl::setassoc_table<
//      std::pair< AddrType, LineType >, 
//      shttl::hasher,
//      shttl::lru_list< Addr, u8 >
//  > table_with_small_associativity.
//
// --- 'setassoc_table_strage_map' is used when the number of associativity is large.
//  shttl::setassoc_table<
//      CachePair, 
//      CacheHasher,
//      shttl::lru_list< Addr, u32 >,
//      shttl::setassoc_table_strage_map<       
//          shttl::setassoc_table_line_base< CachePair >
//      >
//  > table_with_large_associativity.
//

#ifndef SETASSOC_TABLE_H
#define SETASSOC_TABLE_H

#include <stdexcept>
#include <algorithm>
#include <functional>
#include <cassert>
#include <vector>
#include <map>


#include "shttl_types.h"
#include "std_hasher.h"
#include "lru.h"
#include "nlu.h"


namespace shttl
{
    // This iterator refers a line by the id of the line, which comprises the 
    // index and the way number of the line.
    template < typename BodyType, typename BodyPtrType, typename LineType >
    class setassoc_table_iterator_base
    {
    public:

        typedef BodyType    body_type;
        typedef BodyPtrType body_ptr_type;
        typedef LineType    line_type;
        typedef typename body_type::size_type size_type;

        static const size_type invalid_index = body_type::invalid_index;
        static const size_type invalid_way   = body_type::invalid_way;

        setassoc_table_iterator_base( 
            body_ptr_type body  = NULL,
            size_type     index = invalid_index, 
            size_type     way   = invalid_way
        ) : 
            m_body ( body ),
            m_index( index ),
            m_way  ( way   )
        {
        }

        size_type way() const
        {
            return m_way;
        }

        size_type index() const 
        {
            return m_index;
        }

        // Returns the pointer of a table body (setassoc_table).
        const body_type* body() const
        {
            return m_body;
        }

        bool operator != ( const setassoc_table_iterator_base& rhs ) const
        {
            return !((*this) == rhs);
        }

        bool operator == ( const setassoc_table_iterator_base& rhs ) const
        {
            return 
                ( m_body  == rhs.m_body  ) &&
                ( m_index == rhs.m_index ) &&
                ( m_way   == rhs.m_way   );
        }

        // Pre-increment
        setassoc_table_iterator_base& operator++()
        {   
            SHTTL_ASSERT( m_index < m_body->set_num() );
            SHTTL_ASSERT( m_way   < m_body->way_num() );
            m_way++;
            if( m_way == m_body->way_num() ){
                m_way = 0;
                m_index++;
            }
            return *this;
        }

        // Post-increment
        setassoc_table_iterator_base operator++(int)
        {   
            setassoc_table_iterator_base tmp = *this;
            ++*this;
            return tmp;
        }

        line_type* operator->() 
        {
            return &m_body->at( *this );
        }

        line_type& operator*()
        {
            return m_body->at( *this );
        }

    protected:

        body_ptr_type m_body;   // The pointer of a table body.
        size_type     m_index;
        size_type     m_way;

    };

    // A const version iterator.
    template < 
        typename BodyType, 
        typename BodyPtrType, 
        typename LineType,
        typename IteratorType 
    >
    class setassoc_table_const_iterator_base : 
        public setassoc_table_iterator_base< BodyType, BodyPtrType, LineType >
    {
        
    public:

        typedef
            setassoc_table_iterator_base< 
                BodyType, 
                BodyPtrType, 
                LineType 
            > 
            base_type;

        typedef
            setassoc_table_const_iterator_base<
                BodyType, 
                BodyPtrType,
                LineType, 
                IteratorType
            >
            this_type;

        typedef IteratorType iterator;
        typedef typename base_type::body_ptr_type body_ptr_type;
        typedef typename base_type::size_type size_type;
        typedef typename base_type::line_type line_type;


        setassoc_table_const_iterator_base(
            body_ptr_type body  = NULL,
            size_type     index = base_type::invalid_index, 
            size_type     way   = base_type::invalid_way
        ) : base_type( body, index, way )
        {
        }

        setassoc_table_const_iterator_base(
            const iterator &ref
        ) : 
            base_type( ref.body(), ref.index(), ref.way() )
        {
        }

        bool operator != ( const this_type& rhs ) const
        {
            return *static_cast<const base_type*>(this) != rhs;
        }

        bool operator == ( const this_type& rhs ) const
        {
            return *static_cast<const base_type*>(this) == rhs;
        }

        line_type* operator->() 
        {
            return &m_body->at( *this );
        }

        line_type& operator*() 
        {
            return m_body->at( *this );
        }

    protected:
        using base_type::m_body;

    };


    //
    // Cache sets in setassoc_table.
    // 'setassoc_table_set_vector' and 'setassoc_table_storage_vector' 
    // work cooperatively.
    // This class is a version using 'std::vector'.
    // 'find()' is implemented by linear search, and thus
    // this class is fast when a table has small associativity.
    //
    template <
        typename line_type, 
        typename strage_type 
    >
    class setassoc_table_set_vector
    {

    public:

        typedef typename strage_type::size_type size_type;
        typedef typename line_type::tag_type    tag_type;
        typedef typename line_type::value_type  value_type;

        static const size_type invalid_index = strage_type::invalid_index;
        static const size_type invalid_way   = strage_type::invalid_way;

        setassoc_table_set_vector( 
            strage_type* lines, 
            size_type way_num, 
            size_type offset 
        ) :
            m_strage ( lines ),
            m_way_num( way_num ),
            m_offset ( offset )
        {
        }

        // Finds and returns a way number if its tag equals 'tag'.
        size_type find( const tag_type tag ) const
        {
            for( size_type w = 0; w < m_way_num; w++ ){
                const line_type& line = at( w );
                // Validness must be checked first for avoiding to call an user 
                // defined '==' operator with an invalid line.
                if( line.valid && line.tag == tag ){
                    return w;
                }
            }
            return invalid_way;
        }

        // Find and returns an invalid way.
        size_type find_free_way() const
        {
            for( size_type w = 0; w < m_way_num; w++ ){
                if( !at( w ).valid )
                    return w;
            }
            return invalid_way;
        }

        // Invalidate 'way'th line.
        void invalidate( const size_type way )
        {
            if( way == invalid_way )
                return;

            assert_valid_way( way );
            at( way ).valid = false;
        }


        bool read( size_type way, line_type* line ) const
        {
            if( way == invalid_way )
                return false;

            assert_valid_way( way );
            if( line ){
                *line = at( way );
            }
            return at( way ).valid;
        }

        void write(
            const size_type way,
            const tag_type  tag, 
            const value_type &val = value_type()
        ){
            if(way == invalid_way)
                return;

            assert_valid_way(way);

            // keyの重複？
            SHTTL_ASSERT( find(tag) == invalid_way || find( tag ) == way );

            // 書き込み
            at( way ) = line_type( tag, val, true );
        }

        // Invalidate all lines in this set.
        void clear()
        {
            for( size_type w = 0; w < m_way_num; w++ ){
                at( w ).valid = false;
            }
        }

        line_type& at( size_type way )
        {
            return m_strage->at( way + m_offset );
        }

        const line_type& at( size_type way ) const 
        {
            return m_strage->at( way + m_offset );
        }

    protected:

        strage_type* m_strage;
        size_type m_way_num;
        size_type m_offset;

        void assert_valid_way( size_type way ) const
        {
            SHTTL_ASSERT( 0 <= way && way < m_way_num );
        }

    };

    template < typename line_type >
    class setassoc_table_strage_vector
    {
    public:

        typedef setassoc_table_strage_vector< line_type > this_type;
        typedef size_t size_type;
        typedef std::vector<line_type> strage_type;

        typedef setassoc_table_set_vector<
            line_type,
            this_type
        > set_type;
        
        typedef setassoc_table_set_vector< 
            const line_type, 
            const this_type 
        > const_set_type;

        static const size_type invalid_index = ~((size_type)0);
        static const size_type invalid_way   = ~((size_type)0);

        line_type& at( size_type index )
        {
            return m_body.at( index );
        }

        const line_type& at( size_type index ) const 
        {
            return m_body.at( index );
        }

        set_type get_set( size_type index )
        {
            size_type offset = index*m_way_num;
            return set_type( 
                this,
                m_way_num,
                offset
            );
        }

        const_set_type get_set( size_type index ) const
        {
            size_type offset = index*m_way_num;
            return const_set_type( 
                this,
                m_way_num,
                offset
            );
        }

        void resize( size_type set_num, size_type way_num )
        {
            m_set_num = set_num;
            m_way_num = way_num;
            m_body.resize( set_num*way_num, line_type() );
        }

    protected:

        size_type   m_set_num;
        size_type   m_way_num;
        strage_type m_body;

    };


    //
    // Cache sets in setassoc_table.
    // 'setassoc_table_set_map' and 'setassoc_table_strage_map' 
    // work cooperatively.
    // This class is a version using 'std::map'.
    // 'find()' is implemented by 'std::map' thus
    // this class is fast in a table with large associativity
    //
    template <
        typename line_type, 
        typename strage_type 
    >
    class setassoc_table_set_map
    {

    public:

        typedef typename strage_type::size_type size_type;
        typedef typename line_type::key_type    tag_type;
        typedef typename line_type::value_type  value_type;

        typedef typename strage_type::line_map_type    line_map_type;
        typedef typename strage_type::invalid_map_type invalid_map_type;

        static const size_type invalid_index = strage_type::invalid_index;
        static const size_type invalid_way   = strage_type::invalid_way;

        setassoc_table_set_map( 
            strage_type* lines, 
            size_type way_num, 
            size_type index, 
            size_type offset 
        ) :
            m_strage ( lines ),
            m_way_num( way_num ),
            m_index  ( index ),
            m_offset ( offset )
        {
        }

        // tagに対応するway番号を返す
        size_type find( const tag_type tag ) const
        {
            line_map_type& strage_set = m_strage->get_line_map( m_index );
            typename line_map_type::iterator strage_line = strage_set.find( tag );
            
            size_type found = invalid_way;
            if( strage_line != strage_set.end() && strage_line->second->valid ){
                found = 
                    strage_line->second - m_strage->get_set_strage_iterator( m_index );
            }

        #ifdef SHTTL_SET_ASSOC_TABLE_VALIDATE_STRAGE_MAP
            size_type linear_found = invalid_way;
            for( size_type w = 0; w < m_way_num; w++ ){
                const line_type& line = at( w );
                if( line.key == tag && line.valid ){
                    linear_found = w;
                    break;
                }
            }
            SHTTL_ASSERT( linear_found == found );
        #endif

            return found;
        }

        // 空きwayを返す
        size_type find_free_way() const
        {

            invalid_map_type& invalid_set = m_strage->get_invalid_map( m_index );
            typename invalid_map_type::iterator invalid_line = invalid_set.begin();
            size_type found = invalid_way;
            if( invalid_line != invalid_set.end() ){
                found = invalid_line->second;
            }

        #ifdef SHTTL_SET_ASSOC_TABLE_VALIDATE_STRAGE_MAP
            size_type linear_found = invalid_way;
            for( size_type w = 0; w < m_way_num; w++ ){
                if( !at( w ).valid ){
                    linear_found = w;
                    break;
                }
            }
            SHTTL_ASSERT( linear_found == found );
        #endif

            return found;
        }

        // way番目のwayを無効にする
        void invalidate( const size_type way )
        {
            if( way == invalid_way )
                return;

            assert_valid_way( way );
            at( way ).valid = false;

            m_strage->get_invalid_map( m_index )[ way ] = way;
        }
        
        bool read( size_type way, line_type* line ) const
        {
            if( way == invalid_way )
                return false;

            assert_valid_way( way );
            if( line ){
                *line = at( way );
            }
            return at( way ).valid;
        }

        void write(
            const size_type way,
            const tag_type  tag, 
            const value_type &val = value_type()
        ){
            if(way == invalid_way)
                return;

            assert_valid_way(way);

            // keyの重複？
            SHTTL_ASSERT( find(tag) == invalid_way || find( tag ) == way );

            // 書き込み
            typename strage_type::strage_type::iterator line =
                m_strage->get_set_strage_iterator( m_index ) + way;

            line_map_type& strage_set = m_strage->get_line_map( m_index );
            typename line_map_type::iterator old_line = strage_set.find( at( way ).key );
            if( old_line->second == line ){
                strage_set.erase( old_line );
            }

            at( way ) = line_type( tag, val, true );

            strage_set[ tag ] = line;
            m_strage->get_invalid_map( m_index ).erase( way );
        }

        // Invalidate all lines in this set.
        void clear()
        {
            for( size_type way = 0; way < m_way_num; way++ ){
                at( way ).valid = false;

                m_strage->get_invalid_map( m_index )[ way ] = way;
            }
        }

        line_type& at( size_type way )
        {
            return m_strage->at( way + m_offset );
        }

        const line_type& at( size_type way ) const 
        {
            return m_strage->at( way + m_offset );
        }

    protected:

        strage_type* m_strage;
        size_type m_way_num;
        size_type m_index;
        size_type m_offset;

        void assert_valid_way( size_type way ) const
        {
            SHTTL_ASSERT( 0 <= way && way < m_way_num );
        }

    };

    template < typename line_type >
    class setassoc_table_strage_map
    {
    public:

        typedef setassoc_table_strage_map< line_type > this_type;
        typedef size_t size_type;
        typedef std::vector<line_type> strage_type;
        
        typedef typename line_type::key_type   key_type;
        typedef typename line_type::value_type value_type;

        typedef 
            setassoc_table_set_map<
                line_type,
                this_type
            > 
            set_type;
        
        typedef 
            setassoc_table_set_map< 
                const line_type, 
                const this_type 
            > 
            const_set_type;

        typedef 
            std::map<
                key_type, 
                typename strage_type::iterator
            >
            line_map_type;

        typedef 
            std::map<
                size_type, 
                size_type
            >
            invalid_map_type;

        static const size_type invalid_index = ~((size_type)0);
        static const size_type invalid_way   = ~((size_type)0);

        line_type& at( size_type index )
        {
            return m_body.at( index );
        }

        const line_type& at( size_type index ) const 
        {
            return m_body.at( index );
        }

        set_type get_set( size_type index )
        {
            size_type offset = index*m_way_num;
            return set_type( 
                this,
                m_way_num,
                index,
                offset
            );
        }

        const_set_type get_set( size_type index ) const
        {
            size_type offset = index*m_way_num;
            return const_set_type( 
                this,
                m_way_num,
                index,
                offset
            );
        }

        line_map_type& get_line_map( size_type index ){
            return m_line_map[ index ];
        }
        
        invalid_map_type& get_invalid_map( size_type index ){
            return m_invalid_map[ index ];
        }

        typename strage_type::iterator get_set_strage_iterator( size_type index )
        {
            return m_body.begin() + index * m_way_num; 
        }

        void resize( size_type set_num, size_type way_num )
        {
            m_set_num = set_num;
            m_way_num = way_num;
            
            m_body.clear();
            m_body.resize( set_num*way_num, line_type() );

            m_line_map.clear();
            m_line_map.resize( set_num );

            m_invalid_map.clear();
            m_invalid_map.resize( set_num );

            for( size_type index = 0; index < set_num; index++ ){
                
                set_type  set    = get_set( index );
                line_map_type&    set_line_map    = m_line_map[ index ];
                invalid_map_type& set_invalid_map = m_invalid_map[ index ];
                typename strage_type::iterator set_iterator = get_set_strage_iterator( index );

                // Update map and invalid line information.
                for( size_type way = 0; way < way_num; way++ ){
                    line_type& line = set.at( way );
                    if( line.valid ){
                        set_line_map[ line.key ] = set_iterator + way;
                    }
                    else{
                        set_invalid_map[ way ] = way;
                    }
                }

            }
        }

    protected:

        size_type   m_set_num;
        size_type   m_way_num;

        strage_type m_body;
        
        // Visual Studio 2010 cannot treat types with names whose 
        // length is larger than 4096. These struct are hack to 
        // reduce the length of type name..
        struct line_map_allocator : public std::allocator<line_map_type>
        {
        };
        struct invalid_map_allocator : public std::allocator<invalid_map_type>
        {
        };
        std::vector< line_map_type,    line_map_allocator >    m_line_map;
        std::vector< invalid_map_type, invalid_map_allocator > m_invalid_map;

    };

    // Line data stored in a table.
    template < typename PairType >
    struct setassoc_table_line_base
    {
        typedef typename PairType::first_type  tag_type;
        typedef typename PairType::second_type value_type;

        tag_type   tag;
        value_type value;
        bool       valid;

        setassoc_table_line_base(
            const tag_type&   tag_arg   = tag_type(),
            const value_type& value_arg = value_type(),
            const bool&       valid_arg = false
        ) :
            tag  ( tag_arg   ),
            value( value_arg ),
            valid( valid_arg )
        {
        }
    };


    //
    // Set-associative table.
    //
    template<
        typename PairType,          // The pair of an address and a value.
        typename Hasher = std_hasher< typename PairType::first_type >,
        typename Replacer = lru< typename PairType::first_type >,
        typename Strage =           
            setassoc_table_strage_vector< 
                setassoc_table_line_base< PairType >
            >
    >
    class setassoc_table
    {
    public:

        // Types

        typedef
            setassoc_table< PairType, Hasher, Replacer, Strage > 
            this_type;

        typedef PairType pair_type;
        typedef Hasher   hasher_type;
        typedef Replacer replacer_type;
        typedef Strage   strage_type;
        
        typedef typename pair_type::first_type   key_type;
        typedef typename pair_type::second_type  value_type;
        typedef typename hasher_type::size_type  hasher_size_type;

        typedef typename strage_type::set_type       set_type;
        typedef typename strage_type::const_set_type const_set_type;
        typedef typename strage_type::size_type      size_type;

        static const size_type invalid_index = strage_type::invalid_index;
        static const size_type invalid_way   = strage_type::invalid_way;

        typedef 
            setassoc_table_line_base< pair_type > 
            line_type;

        // iterator and const iterator.
        typedef 
            setassoc_table_iterator_base<
                this_type, 
                this_type*,
                line_type
            > 
            iterator;

        typedef 
            setassoc_table_const_iterator_base<
                this_type,
                const this_type*, 
                const line_type, 
                iterator
            > 
            const_iterator;



        // Accessors
        size_type set_num() const
        {
            return (size_type)m_hasher.size();
        }

        size_type way_num() const
        { 
            return m_way_num; 
        }

        // size in lines
        size_type size() const
        {
            return set_num() * way_num();
        }


        // Constructors
        setassoc_table(
            const hasher_type &hasher,
            const size_type way_num
        ) :
            m_replacer(),
            m_hasher  ( hasher  ),
            m_way_num ( way_num )
        {
            m_replacer.construct( hasher.size(), way_num ); 
            m_strage.resize( hasher.size(), way_num );
        }

        ~setassoc_table()
        {
        }
        
        // Returns an iterator of a value that corresponds the key.
        iterator find( const key_type key )
        {
            hasher_size_type index = m_hasher.index( key );
            key_type tag = m_hasher.tag( key );

            const set_type& set = m_strage.get_set( (size_type)index );
            size_type way = set.find( tag );
            if( way != invalid_way )
                return iterator( this, index, way );
            else
                return end();
        }

        // Returns an iterator of a value that corresponds the key.
        iterator read( const key_type key, value_type* val = NULL )
        {
            iterator id = find( key );

            // hit
            if( id != end() ){
                if( val ){
                    *val = at( id ).value;
                }
                touch( id, key );
            }

            return id;
        }

        // Returns an iterator of a value that corresponds the key.
        iterator write(
            const key_type   key, 
            const value_type &val    = value_type(),
            bool*      ref_replaced      = NULL,
            key_type*  ref_replaced_key = NULL,
            line_type* ref_replaced_line = NULL
        ){
            hasher_size_type key_index = index( key );
            set_type set = m_strage.get_set( key_index );
            bool replaced = false;

            // miss
            iterator id = find( key );
            if( id == end() ){

                // Find an invalid way.
                size_type way = set.find_free_way();

                // Could not find a free way and now replace a valid way.
                if( way == invalid_way ){
                    way = m_replacer.target( key_index );
                    replaced = true;
                }

                id = iterator( this, key_index, way );
            }

            if( ref_replaced ){
                *ref_replaced = replaced;
            }

            if( replaced && ref_replaced_line ){
                *ref_replaced_line = set.at( id.way() );
            }

            if( replaced && ref_replaced_key ){
                *ref_replaced_key = m_hasher.rebuild( ref_replaced_line->tag, id.index() );
            }

            key_type tag = m_hasher.tag( key );
            set.write( id.way(), tag, val );

            touch( id, key );
            return id;
        }

        // true if hit
        iterator invalidate( const key_type key )
        {
            iterator id = find( key );
            if( id != end() && at(id).valid ){
                m_strage.get_set( id.index() ).invalidate( id.way() );
            }
            return id;
        }

        void touch( iterator id, const key_type key )
        {
            m_replacer.touch( id.index(), id.way(), key );
        }

        void clear()
        {
            for( size_type i = 0; i < set_num(); i++ ){
                m_strage.get_set( i ).clear();
            }
        }

        hasher_size_type index( const key_type key ) const
        {
            return m_hasher.index( key );
        }

        //
        // Access interfaces
        //
        line_type& at( iterator id )
        {
            return m_strage.get_set( id.index() ).at( id.way() );
        };

        const line_type& at( const_iterator id ) const
        {
            return m_strage.get_set( id.index() ).at( id.way() );
        };

        iterator begin()
        {
            return iterator( this, 0, 0 );
        }

        iterator end()
        {
            return iterator( this, set_num(), 0 );
        }

        const_iterator begin() const
        {
            return const_iterator( this, 0, 0 );
        }

        const_iterator end() const
        {
            return const_iterator( this, set_num(), 0 );
        }

        iterator begin_set( hasher_size_type index )
        {
            return iterator( this, index, 0 );
        }

        iterator end_set( hasher_size_type index )
        {
            return iterator( this, index+1, 0 );
        }

        const_iterator begin_set( hasher_size_type index ) const
        {
            return const_iterator( this, index, 0 );
        }

        const_iterator end_set( hasher_size_type index ) const
        {
            return const_iterator( this, index+1, 0 );
        }


        //
        // setassoc. table only
        //
        replacer_type& get_replacer()
        {
            return m_replacer;
        };

        hasher_type& get_hasher()
        {
            return m_hasher;
        }


    protected:

        replacer_type m_replacer;       // replacer object
        hasher_type   m_hasher;     // key hasher
        size_type     m_way_num;

        strage_type m_strage;       // tag & value & valid

        // Copying setassoc_table is not allowed.
        setassoc_table(
            const setassoc_table& ref
        )
        {
        }


    }; // class setassoc_table

} // namespace shttl

#endif // SETASSOC_TABLE_H
