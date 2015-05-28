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


#include <pch.h>
#include "Sim/Op/OpContainer/OpList.h"
#include "Sim/Op/Op.h"

using namespace Onikiri;

OpList::OpList()
{
}

OpList::OpList( const OpArray& opArray )
{
    resize( opArray );
}

void OpList::resize( const OpArray& opArray )
{
    resize( opArray.GetCapacity() );
}

void OpList::resize( int capacity )
{
    alive_table.resize( capacity, false );
    iterator_table.resize( capacity );
}

OpList::~OpList()
{
}

OpList::iterator OpList::get_iterator( const OpIterator& opIterator ) const
{
    return iterator_table[ opIterator.GetID() ];
}

OpList::iterator OpList::operator[]( const OpIterator& opIterator ) const
{
    return get_iterator( opIterator );
}

OpList::iterator OpList::insert( iterator pos, const OpIterator& opIterator )
{
    OpArray::ID id     = opIterator.GetID();

    ASSERT(
        alive_table[ id ] == false, 
        "opIterator inserted twice."
    );
    
    iterator iter = 
        list_type::insert( pos, opIterator );

    iterator_table[ id ] = iter;
    alive_table[ id ] = true;

    return iter;
}

OpList::iterator OpList::erase( const OpIterator& opIterator )
{
    return erase( get_iterator(opIterator) );
}

OpList::iterator OpList::erase( iterator pos )
{
    // 再度同じ handle が insert されるときに初期化されるので、
    // erase する時に m_iteratorTable の該当する要素には何もしない
    OpArray::ID     id     = pos->GetID();

    alive_table[ id ] = false;
    iterator_table[ id ] = end();
    
    return list_type::erase( pos );
}

void OpList::clear()
{
    list_type::clear();
    alive_table.reset();
}

OpList::iterator OpList::find( const OpIterator& opIterator )
{
    if( alive_table[ opIterator.GetID() ] == true ) {
        return get_iterator(opIterator);
    }else {
        return list_type::end();
    }
}

size_t OpList::count( const OpIterator& op ) const
{
    if( alive_table[ op.GetID() ] == true ) {
        return 1;
    }else {
        return 0;
    }
}

bool OpList::find_and_erase( OpIterator op )
{
    iterator i = find(op);
    bool erased = i != end();
    if(erased){
        erase(i);
    }
    return erased;
}

void OpList::push_inorder( OpIterator op )
{
    // 空であればpush_backして終わり
    if(empty()) {
        push_back(op);
        return;
    }

    // 後ろ側からopよりSerialIDが小さいOpIteratorを見つける
    iterator iter = end();
    u64 id = op->GetGlobalSerialID();
    do {
        --iter;
        if( (*iter)->GetGlobalSerialID() < id ) {
            // 見つけたOpIteratorの後ろにopを挿入
            // insert は指定した要素の直前に挿入なので1回インクリメント
            ++iter;
            insert(iter, op);
            return;
        }
    } while( iter != begin() );
    
    // 先頭まで達したら先頭に入れる
    push_front(op);
}

