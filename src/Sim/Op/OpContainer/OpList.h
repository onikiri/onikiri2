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


#ifndef SIM_OP_OP_CONTAINER_OP_LIST_H
#define SIM_OP_OP_CONTAINER_OP_LIST_H

#include "Utility/RuntimeError.h"
#include "Sim/Op/OpArray/OpArray.h"

namespace Onikiri 
{
    class OpList
        : private pool_list<OpIterator>
    {
    public:
        typedef pool_list<OpIterator> list_type;
        typedef pool_list<OpIterator>::iterator iterator;
        typedef pool_list<OpIterator>::const_iterator const_iterator;

        OpList();
        OpList( const OpArray& opArray );
        virtual ~OpList();

        void resize( const OpArray& opArray );
        void resize( int capacity );

        iterator get_iterator( const OpIterator& opIterator ) const;
        iterator get_original_iterator( const OpIterator& opIterator ) const;
        iterator operator[]( const OpIterator& opIterator ) const;
        iterator insert( iterator pos, const OpIterator& opIterator );
        iterator erase( const OpIterator& opIterator );
        iterator erase( iterator pos );
        void clear();
        iterator find( const OpIterator& opIterator );
        size_t count( const OpIterator& op ) const;
        void push_inorder( OpIterator op );
        bool find_and_erase( OpIterator op );

        size_t size() const
        {
            return list_type::size();
        }

        bool empty() const
        {
            return list_type::empty();
        }

        iterator begin()
        {
            return list_type::begin();
        }

        const_iterator begin() const
        {
            return list_type::begin();
        }

        iterator end()
        {
            return list_type::end();
        }

        const_iterator end() const
        {
            return list_type::end();
        }

        OpIterator& front()
        {
            return list_type::front();
        }

        OpIterator& back()
        {
            return list_type::back();
        }

        const OpIterator& front() const 
        {
            return list_type::front();
        }

        const OpIterator& back() const 
        {
            return list_type::back();
        }

        void push_front( const OpIterator& opIterator )
        {
            insert( begin(), opIterator );
        }

        void push_back( const OpIterator& opIterator )
        {
            insert( end(), opIterator );
        }

        void pop_front()
        {
            erase( begin() );
        }

        void pop_back()
        {
            erase( --end() );
        }

        template <class SortCmpT>
        void sort(SortCmpT cmp)
        {
            list_type::sort(cmp);
        }

        // Move ops from other list to this list.
        void move( OpList* from )
        {
            for( iterator i = from->begin(); i != from->end(); ){
                push_inorder( *i );
                i = from->erase(i);
            }
        }

    protected:
        boost::dynamic_bitset<> alive_table;
        std::vector< iterator > iterator_table;

    };
}; // namespace Onikiri

#endif // SIM_OP_OP_CONTAINER_OP_LIST_H

