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

#include "Emu/Utility/System/Memory/HeapAllocator.h"

using namespace std;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;

HeapAllocator::HeapAllocator(u64 pageSize) : m_pageSize(pageSize)
{
}

bool HeapAllocator::AddMemoryBlock(u64 start, u64 length)
{
    MemoryBlock mb;
    mb.Addr = start;
    mb.Bytes = length;

    // 重複チェック
    typedef list<MemoryBlock>::iterator iterator;
    for (iterator e = m_freeList.begin(); e != m_freeList.end(); ++e) {
        if (e->Intersects(mb))
            return false;
    }
    for (iterator e = m_allocList.begin(); e != m_allocList.end(); ++e) {
        if (e->Intersects(mb))
            return false;
    }
    m_freeList.push_back(mb);

    return true;
}

// addr は確保されている領域と交差するか
u64 HeapAllocator::IsIntersected(u64 addr, u64 length) const
{
    MemoryBlock mb;
    mb.Addr = addr;
    mb.Bytes = length;
    typedef list<MemoryBlock>::const_iterator const_iterator;
    for (const_iterator e = m_allocList.begin(); e != m_allocList.end(); ++e) {
        if (e->Intersects(mb))
            return true;
    }
    return false;
}

u64 HeapAllocator::Alloc(u64 addr, u64 length)
{
    if (addr != 0)
        return 0;
    
    // legth is expanded to a page boundary.
    length = in_pages(length) * m_pageSize;

    typedef list<MemoryBlock>::iterator iterator;
    for (iterator e = m_freeList.begin(); e != m_freeList.end(); ++e) {
        // first fit
        if (e->Bytes > length) {
            // メモリを確保

            MemoryBlock mb;
            mb.Addr = e->Addr;
            mb.Bytes = length;

            BlockList::iterator alloc_ins_pos = lower_bound(m_allocList.begin(), m_allocList.end(), mb);
            m_allocList.insert(alloc_ins_pos, mb);

            e->Addr = e->Addr + length;
            e->Bytes = e->Bytes - length;
            if (e->Bytes == 0)
                m_freeList.erase(e);
            
            return mb.Addr;
        }
    }

    // メモリ確保に失敗
    return 0;
}

u64 HeapAllocator::ReAlloc(u64 addr, u64 old_size, u64 new_size)
{
    // メモリブロックのサイズを変えない場合
    if (old_size == new_size)
        return addr;

    typedef list<MemoryBlock>::iterator iterator;

    // legth is expanded to a page boundary.
    new_size = in_pages(new_size) * m_pageSize;
    BlockList::iterator alloc_it = find(m_allocList.begin(), m_allocList.end(), MemoryBlock(addr));

    // そんなメモリブロックはない
    if (alloc_it == m_allocList.end())
        return 0;

    if (new_size < old_size) {
        // メモリブロックを小さくする場合

        // 空き領域を追加
        MemoryBlock free_mb;
        free_mb.Addr = alloc_it->Addr+new_size;
        free_mb.Bytes = alloc_it->Bytes-new_size;

        BlockList::iterator free_ins_pos = lower_bound(m_freeList.begin(), m_freeList.end(), free_mb);
        m_freeList.insert(free_ins_pos, free_mb);

        // メモリブロックを小さくする
        alloc_it->Bytes = new_size;

        IntegrateFreeBlocks();
    }
    else {
        // メモリブロックを大きくする場合
        MemoryBlock oldmb;
        oldmb.Addr = addr;
        oldmb.Bytes = old_size;
        // 直後の空きメモリブロックを探す
        iterator next_free = upper_bound(m_freeList.begin(), m_freeList.end(), oldmb);
        iterator next_alloc = upper_bound(m_allocList.begin(), m_allocList.end(), oldmb);

        // 後ろには空きメモリがない
        if (next_free == m_freeList.end())
            return 0;
        // 直後のメモリブロックはallocated
        if (next_alloc != m_allocList.end() && next_alloc->Addr < next_free->Addr)
            return 0;

        // メモリ足りない
        if (alloc_it->Bytes + next_free->Bytes < new_size)
            return 0;

        // remap
        next_free->Bytes -= new_size - alloc_it->Bytes;
        next_free->Addr += new_size - alloc_it->Bytes;
        alloc_it->Bytes = new_size;

        if (next_free->Bytes == 0)
            m_freeList.erase(next_free);
    }
    return alloc_it->Addr;
}


bool HeapAllocator::Free(u64 addr)
{
    BlockList::iterator alloc_it = find(m_allocList.begin(), m_allocList.end(), MemoryBlock(addr));

    // そんなメモリブロックはない
    if (alloc_it == m_allocList.end())
        return false;

    return Free(alloc_it->Addr, alloc_it->Bytes);
}

bool HeapAllocator::Free(u64 addr, u64 size)
{
    // まだAllocされていない
    if (m_allocList.size() == 0)
        return false;

    // allocListから free_mb : [addr, addr+size) を含むメモリブロックを探す
    MemoryBlock free_mb(addr, size);
    // addr 以下のアドレスを持つメモリブロックで，一番最後のものが候補
    BlockList::iterator alloc_it = --upper_bound(m_allocList.begin(), m_allocList.end(), free_mb);
    // free_mb を含むメモリブロックが存在しない
    if (alloc_it->Contains( free_mb ))
        return false;

    // free_mb を Free することにより alloc_itのメモリブロックが3つに分かれる

    // free_mb の後ろ
    u64 free_mb_end = free_mb.Addr+free_mb.Bytes;
    MemoryBlock alloc_mb2( free_mb_end , alloc_it->Addr+alloc_it->Bytes - free_mb_end );
    if (alloc_mb2.Bytes != 0)
        m_allocList.insert(++BlockList::iterator(alloc_it), alloc_mb2);
    
    // free_mb の前
    alloc_it->Bytes = addr - alloc_it->Addr;
    if (alloc_it->Bytes == 0)
        m_allocList.erase(alloc_it);

    // free_mb
    BlockList::iterator free_ins_pos = lower_bound(m_freeList.begin(), m_freeList.end(), free_mb);
    m_freeList.insert(free_ins_pos, free_mb);

    IntegrateFreeBlocks();

    return true;
}

// addr にAllocされたメモリ領域のサイズを得る
u64 HeapAllocator::GetBlockSize(u64 addr) const
{
    BlockList::const_iterator alloc_it = find(m_allocList.begin(), m_allocList.end(), MemoryBlock(addr));

    if (alloc_it == m_allocList.end())
        return 0;
    else
        return alloc_it->Bytes;
}

// 空き領域の統合
void HeapAllocator::IntegrateFreeBlocks()
{
    typedef list<MemoryBlock>::iterator iterator;

    // 空き領域を先頭から見て，それぞれに対し次の空き領域が直後に存在すれば結合する
    for (iterator e = m_freeList.begin(); e != m_freeList.end(); ++e) {
        iterator next;
        for (next = e, ++next; next != m_freeList.end(); next = e, ++next) {
            if (e->Addr + e->Bytes == next->Addr) {
                e->Bytes += next->Bytes;
                m_freeList.erase(next);
            }
            else {
                break;
            }
        }
    }
}

//void HeapAllocator::Dump() const
//{
//  for (iterator e = m_allocList.begin(); e != m_allocList.end(); ++e) {
//      cerr << "(" << e->Addr << ", " << e->Bytes << ") ";
//  }
//  cerr << endl;
//  for (iterator e = m_freeList.begin(); e != m_freeList.end(); ++e) {
//      cerr << "(" << e->Addr << ", " << e->Bytes << ") ";
//  }
//  cerr << endl;
//}
