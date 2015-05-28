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
#include "Emu/Utility/System/Memory/VirtualMemory.h"
#include "SysDeps/Endian.h"

using namespace std;
using namespace Onikiri;
using namespace EmulatorUtility;

// PageTable を引く際に TLB を使用するかどうか
#define ENABLED_EMULATOR_UTILITY_TLB 1

// ターゲット用のmemcpy などのヘルパを ReadMemory や WriteMemory を使わずに
// 高速な実装を使用するかどうか
#define ENABLED_VIRTUAL_MEMORY_FAST_HELPER 1

// The initial bucket count of the unordered_map used in PageTable.
// Note: Too large bucket count (ex. 4096) makes the finalize 
// of the unordered_map very slow in VS 2010.
static const int PAGE_TABLE_MAP_INITIAL_BUCKET_COUNT = 16;


//
// Memory helper function
//

//
// 1-entry TLB
//
TLB::TLB( int offsetBits )
{
    memset( &m_body, 0, sizeof(m_body) );
    m_addr = 0;
    m_valid = false;

    m_offsetBits = offsetBits;
    m_addrMask = shttl::mask( m_offsetBits, 64 - m_offsetBits );
}

TLB::~TLB()
{
}

bool TLB::Lookup( u64 addr, PageTableEntry* entry ) const
{
#ifndef ENABLED_EMULATOR_UTILITY_TLB
    return false;
#endif

    if( m_valid && m_addr == ( addr & m_addrMask ) ){
        *entry = m_body;
        return true;
    }

    return false;
}

void TLB::Write( u64 addr, const PageTableEntry& entry )
{
    m_addr = addr & m_addrMask;
    m_body = entry;
    m_valid = true;
}

void TLB::Flush()
{
    m_valid = false;
}


//
//
//
PageTable::PageTable(int offsetBits)    : 
    m_map(
        PAGE_TABLE_MAP_INITIAL_BUCKET_COUNT, 
        AddrHash(offsetBits)
    ), 
    m_offsetBits(offsetBits), 
    m_offsetMask(~(u64)0 << offsetBits),
    m_tlb(offsetBits),
    m_phyPagePool( sizeof(PhysicalMemoryPage) )
{
}

PageTable::~PageTable()
{
    // Release PhysicalMemoryPage instances.
    for( map_type::iterator i = m_map.begin(); i != m_map.end(); ++i ){
        PageTableEntry&     logPage = i->second;
        PhysicalMemoryPage* phyPage = logPage.phyPage;
        phyPage->refCount--;
        if( phyPage->refCount == 0 ){
            m_phyPagePool.free( phyPage );
        }
    }
    m_map.clear();
}


int PageTable::GetOffsetBits() const
{
    return m_offsetBits;
}

// アドレスのページ外部分のみのマスク
u64 PageTable::GetOffsetMask() const
{
    return m_offsetMask;
}

size_t PageTable::GetPageSize() const
{
    return (size_t)1 << m_offsetBits;
}

void *PageTable::TargetToHost(u64 targetAddr) 
{
    PageTableEntry entry;
    if( m_tlb.Lookup( targetAddr, &entry ) ){
        return static_cast<u8*>(entry.phyPage->ptr) + (targetAddr & ~m_offsetMask);
    }

    map_type::const_iterator e = m_map.find(targetAddr & m_offsetMask);

    if (e == m_map.end()) {
        THROW_RUNTIME_ERROR("unassigned page referred.");
        return 0;
    }
    else{
        // Update TLB
        entry = e->second;
        m_tlb.Write( targetAddr, entry );
        return static_cast<u8*>(entry.phyPage->ptr) + (targetAddr & ~m_offsetMask);
    }
}

void PageTable::AddMap( u64 targetAddr, u8* hostAddr, VIRTUAL_MEMORY_ATTR_TYPE attr )
{
    PageTableEntry      logPage;
    PhysicalMemoryPage* phyPage = (PhysicalMemoryPage*)m_phyPagePool.malloc();
    
    phyPage->ptr = hostAddr;
    phyPage->refCount = 1;

    logPage.phyPage = phyPage;
    logPage.attr = attr;
    
    m_map[ targetAddr & m_offsetMask ] = logPage;
    m_tlb.Flush();
}

// Copy address mapping for copy-on-write.
void PageTable::CopyMap( u64 dstTargetAddr, u64 srcTargetAddr, VIRTUAL_MEMORY_ATTR_TYPE dstAttr )
{
    if( !IsMapped( srcTargetAddr ) ){
        THROW_RUNTIME_ERROR("An unassigned page is copied.");
    }

    map_type::iterator e = m_map.find( srcTargetAddr & m_offsetMask );
    e->second.phyPage->refCount++;

    PageTableEntry newEntry;
    newEntry.phyPage = e->second.phyPage;
    newEntry.attr    = dstAttr; // Set new attribute.
    m_map[ dstTargetAddr & m_offsetMask ] = newEntry;

    m_tlb.Flush();
}

bool PageTable::GetMap( u64 targetAddr, PageTableEntry* page ) 
{
    if( m_tlb.Lookup( targetAddr, page ) ){
        return true;
    }

    map_type::const_iterator e = m_map.find(targetAddr & m_offsetMask);
    if (e == m_map.end()){
        return false;
    }
    else{
        m_tlb.Write( targetAddr, e->second );
        *page = e->second;
        return true;
    }
}

bool PageTable::SetMap( u64 targetAddr, const PageTableEntry& page )
{
    map_type::iterator e = m_map.find(targetAddr & m_offsetMask);
    if (e == m_map.end()){
        return false;
    }
    else{
        m_tlb.Write( targetAddr, page );
        e->second = page;
        return true;
    }
}

// Get a reference count of a page including targetAddr
int PageTable::GetPageReferenceCount( u64 targetAddr )
{
    PageTableEntry page;
    if( GetMap( targetAddr, &page ) ){
        return page.phyPage->refCount;
    }
    else{
        return 0;
    }
}

int PageTable::RemoveMap(u64 targetAddr)
{
    map_type::iterator e = m_map.find(targetAddr & m_offsetMask);
    
    int refCount = 0;
    if( e != m_map.end() ){
        PhysicalMemoryPage* phyPage = e->second.phyPage;
        phyPage->refCount--;
        refCount = phyPage->refCount;
        if( refCount == 0 ){
            m_phyPagePool.free( phyPage );
        }
        m_map.erase(e);
        m_tlb.Flush();
        return refCount;
    }
    return -1;
}

bool PageTable::IsMapped(u64 targetAddr) const
{
    PageTableEntry entry;
    if( m_tlb.Lookup( targetAddr, &entry ) ){
        return true;
    }

    return (m_map.find(targetAddr & m_offsetMask) != m_map.end());
}


//
// VirtualMemory
//

// addr から size バイトのメモリ領域を，マップ単位境界で分割する
// 結果は，MemoryBlockのコンテナへのイテレータ Iter を通して格納する
// 戻り値は分割された個数
            
// ※ Iterは，典型的にはMemoryBlockのコンテナのinserter
template <typename Iter>
int VirtualMemory::SplitAtMapUnitBoundary(u64 addr, u64 size, Iter e) const
{
    u64 unitSize = GetPageSize();
    u64 endAddr = addr+size;
    u64 offsetMask = m_pageTbl.GetOffsetMask();
    int nBlocks = 0;

    // 最初の半端なブロック
    u64 firstSize = std::min(unitSize - (addr & ~offsetMask), endAddr - addr);
    *e++ = MemoryBlock(addr, firstSize);
    addr += firstSize;
    nBlocks ++;

    // 1マップ単位丸ごと占めるブロックと最後
    while (addr < endAddr) {
        *e++ = MemoryBlock(addr, std::min(unitSize, endAddr-addr) );
        addr += unitSize;
        nBlocks ++;
    }

    return nBlocks;
}


VirtualMemory::VirtualMemory( int pid, bool bigEndian, SystemIF* simSystem ) : 
    m_pageTbl( VIRTUAL_MEMORY_PAGE_SIZE_BITS ),     // 変換単位のビット数を指定
    m_pool( m_pageTbl.GetPageSize() ),      // m_pageTbl 内で変換単位を計算して返してくれる
    m_simSystem( simSystem ),
    m_pid(pid),
    m_bigEndian(bigEndian)
{
}

VirtualMemory::~VirtualMemory()
{
}

// メモリ管理
u64 VirtualMemory::GetPageSize() const
{
    return m_pageTbl.GetPageSize();
}

void VirtualMemory::ReadMemory( MemAccess* access ) 
{
    u64 addr = access->address.address;
    PageTableEntry page;

    // 投機的な読み出しは，アクセス違反となっている場合もあるので，終了してはいけない．
    if( !m_pageTbl.GetMap( addr, &page ) ) {
        access->result = MemAccess::MAR_READ_INVALID_ADDRESS;
        access->value = 0;
        return;
    }

    // アクセスがマップ単位境界をまたいでいないか
    if ((addr ^ (addr+access->size-1)) >> VIRTUAL_MEMORY_PAGE_SIZE_BITS != 0) {
        access->result = MemAccess::MAR_READ_UNALIGNED_ADDRESS;
        access->value = 0;
        return;
    }
    
    // Check attribute
    if( !(page.attr & VIRTUAL_MEMORY_ATTR_READ) ){
        access->result = MemAccess::MAR_READ_NOT_READABLE;
        return;
    }

    void *ptr = m_pageTbl.TargetToHost( addr );
    u64 result;
    switch (access->size) {
    case 1:
        if (access->sign)
            result = (u64)(s64)EndianSpecifiedToHost(*static_cast<s8*>(ptr), m_bigEndian);
        else
            result = (u64)EndianSpecifiedToHost(*static_cast<u8*>(ptr), m_bigEndian);
        break;
    case 2:
        if (access->sign)
            result = (u64)(s64)EndianSpecifiedToHost(*static_cast<s16*>(ptr), m_bigEndian);
        else
            result = (u64)EndianSpecifiedToHost(*static_cast<u16*>(ptr), m_bigEndian);
        break;
    case 4:
        if (access->sign)
            result = (u64)(s64)EndianSpecifiedToHost(*static_cast<s32*>(ptr), m_bigEndian);
        else
            result = (u64)EndianSpecifiedToHost(*static_cast<u32*>(ptr), m_bigEndian);
        break;
    case 8:
        result = EndianSpecifiedToHost(*static_cast<u64*>(ptr), m_bigEndian);
        break;
    default:
        THROW_RUNTIME_ERROR("Invalid Read Size");
        result = 0; // to suppress "result is uninitialized" warning
    }

    access->result = MemAccess::MAR_SUCCESS;
    access->value = result;
}

void VirtualMemory::WriteMemory( MemAccess* access )
{
    PageTableEntry page;
    u64 addr = access->address.address;
    if( !m_pageTbl.GetMap( addr, &page ) ) {
        access->result = MemAccess::MAR_WRITE_INVALID_ADDRESS;
        return;
    }

    // アクセスがマップ単位境界をまたいでいないか
    if ((addr ^ (addr+access->size-1)) >> VIRTUAL_MEMORY_PAGE_SIZE_BITS != 0) {
        access->result = MemAccess::MAR_WRITE_UNALIGNED_ADDRESS;
        return;
    }

    if( !(page.attr & VIRTUAL_MEMORY_ATTR_WRITE) ){
        access->result = MemAccess::MAR_WRITE_NOT_WRITABLE;
        return;
    }

    if( CopyPageOnWrite( addr ) ){
        // copy-on-writ 時はテーブルの参照先が更新されるので，もう一度テーブルエントリを得る
        // Get a page table entry again because a reference in the page table entry
        // is updated on copy-on-write.
        m_pageTbl.GetMap( addr, &page );
    }

    void *ptr = m_pageTbl.TargetToHost( addr );
    switch (access->size) {
    case 1:
        *static_cast<u8*>(ptr) = EndianHostToSpecified((u8)access->value, m_bigEndian);
        break;
    case 2:
        *static_cast<u16*>(ptr) = EndianHostToSpecified((u16)access->value, m_bigEndian);
        break;
    case 4:
        *static_cast<u32*>(ptr) = EndianHostToSpecified((u32)access->value, m_bigEndian);
        break;
    case 8:
        *static_cast<u64*>(ptr) = EndianHostToSpecified((u64)access->value, m_bigEndian);
        break;
    default:
        THROW_RUNTIME_ERROR("Invalid Write Size");
    }

    access->result = MemAccess::MAR_SUCCESS;
}


void VirtualMemory::TargetMemset(u64 targetAddr, int value, u64 size)
{
#ifdef ENABLED_VIRTUAL_MEMORY_FAST_HELPER
    BlockArray blocks;
    SplitAtMapUnitBoundary(targetAddr, size, back_inserter(blocks) );

    for (BlockArray::iterator e = blocks.begin(); e != blocks.end(); ++e) {
        CopyPageOnWrite( e->addr );
        memset(m_pageTbl.TargetToHost(e->addr), value, (size_t)e->size);
    }
#else
    for( u64 i = 0; i < size; i++ ){
        WriteMemory( targetAddr + i, 1, value );
    }
#endif
}

void VirtualMemory::MemCopyToHost(void* dst, u64 src, u64 size) 
{
#ifdef ENABLED_VIRTUAL_MEMORY_FAST_HELPER
    
    BlockArray blocks;
    SplitAtMapUnitBoundary(src, size, back_inserter(blocks) );

    u8* dst_u8 = static_cast<u8*>(dst);
    for (BlockArray::iterator e = blocks.begin(); e != blocks.end(); ++e) {
        memcpy(dst_u8, m_pageTbl.TargetToHost(e->addr), (size_t)e->size);
        dst_u8 += e->size;
    }
#else
    u8* host   = static_cast<u8*>(dst);
    u64 target = src;
    for( u64 i = 0; i < size; i++ ){
        host[i] = static_cast<u8>( ReadMemory( target + i, 1 ) );
    }
#endif
}

void VirtualMemory::MemCopyToTarget(u64 dst, const void* src, u64 size)
{
#ifdef ENABLED_VIRTUAL_MEMORY_FAST_HELPER
    
    BlockArray blocks;
    SplitAtMapUnitBoundary(dst, size, back_inserter(blocks) );

    const u8* src_u8 = static_cast<const u8*>(src);
    for (BlockArray::iterator e = blocks.begin(); e != blocks.end(); ++e) {
        CopyPageOnWrite( e->addr );
        memcpy(m_pageTbl.TargetToHost(e->addr), src_u8, (size_t)e->size);
        src_u8 += e->size;
    }
#else
    const u8* host = static_cast<const u8*>(src);
    u64 target = dst;
    for( u64 i = 0; i < size; i++ ){
        WriteMemory( target + i, 1, host[i] );
    }
#endif
}

void VirtualMemory::AssignPhysicalMemory(u64 addr, VIRTUAL_MEMORY_ATTR_TYPE attr)
{
    if (m_pageTbl.IsMapped(addr)) {
        THROW_RUNTIME_ERROR( "The specified target address is already mapped." );
        //THROW_RUNTIME_ERROR( "%08x%08x is already mapped.", (u32)(addr >> 32), (u32)(addr & 0xffffffff) );
        return;
    }

    void* mem = m_pool.malloc();
    if (!mem)
        THROW_RUNTIME_ERROR("out of memory");
    m_pageTbl.AddMap(addr, (u8*)mem, attr);
    memset(mem, 0, (size_t)GetPageSize());

    m_simSystem->NotifyMemoryAllocation(
        Addr( m_pid, TID_INVALID, addr),
        GetPageSize(),
        true 
    );
}

void VirtualMemory::AssignPhysicalMemory(u64 addr, u64 size, VIRTUAL_MEMORY_ATTR_TYPE attr)
{
    BlockArray blocks;
    SplitAtMapUnitBoundary(addr, size, back_inserter(blocks) );

    for (BlockArray::iterator e = blocks.begin(); e != blocks.end(); ++e){
        AssignPhysicalMemory(e->addr, attr);
    }
}

void VirtualMemory::FreePhysicalMemory(u64 addr)
{
    if(!m_pageTbl.IsMapped(addr)){
        return;
    }

    PageTableEntry page;
    if( m_pageTbl.GetMap( addr, &page ) ){
        // 参照カウンタが1の場合，他に見ている人は居ないので解放
        if( page.phyPage->refCount == 1 && page.phyPage->ptr ){
            m_simSystem->NotifyMemoryAllocation( 
                Addr( m_pid, TID_INVALID, addr), 
                GetPageSize(), 
                false
            );
            m_pool.free( page.phyPage->ptr );
        }
        m_pageTbl.RemoveMap( addr );
    }
}

void VirtualMemory::FreePhysicalMemory(u64 addr, u64 size)
{
    BlockArray blocks;
    SplitAtMapUnitBoundary(addr, size, back_inserter(blocks) );

    for (BlockArray::iterator e = blocks.begin(); e != blocks.end(); ++e){
        FreePhysicalMemory(e->addr);
    }
}

// dstAddr を含むページに srcAddr を含むページに現在割り当てられている物理メモリを割り当てる．
void VirtualMemory::SetPhysicalMemoryMapping( u64 dstAddr, u64 srcAddr, VIRTUAL_MEMORY_ATTR_TYPE attr )
{
    if( !m_pageTbl.IsMapped( srcAddr ) ){
        THROW_RUNTIME_ERROR( "The specified source address is not mapped." );
    }

    // 既に割り当てられているページはスルー
    if( m_pageTbl.IsMapped( dstAddr ) ){
        THROW_RUNTIME_ERROR( "The specified target address is already mapped." );
    }

    m_pageTbl.CopyMap( dstAddr, srcAddr, attr );
}

// [dstAddr, dstAddr+size) を含むページに物理メモリマッピングをコピー．同上
void VirtualMemory::SetPhysicalMemoryMapping( u64 dstAddr, u64 srcAddr, u64 size, VIRTUAL_MEMORY_ATTR_TYPE attr )
{
    BlockArray blocks;
    SplitAtMapUnitBoundary( dstAddr, size, back_inserter(blocks) );
    for (BlockArray::iterator e = blocks.begin(); e != blocks.end(); ++e){
        SetPhysicalMemoryMapping( e->addr, srcAddr, attr );
    }
}

// 論理アドレス addr に新しい物理ページを割り当て，以前指していた物理ページから内容をコピー．
// Copy on Write で使用．
void VirtualMemory::AssignAndCopyPhysicalMemory( u64 addr, VIRTUAL_MEMORY_ATTR_TYPE attr )
{
    PageTableEntry page;
    if( !m_pageTbl.GetMap( addr, &page ) ) {
        THROW_RUNTIME_ERROR( "The specified address is not mapped." );
        return; // To avoid compiler's warning "Uninitialized 'page' is used".
    }

    // Do not need to copy in this case.
    if( page.phyPage->refCount == 1 ){
        return;
    }

    m_pageTbl.RemoveMap( addr );
    AssignPhysicalMemory( addr, attr );
    MemCopyToTarget( addr & m_pageTbl.GetOffsetMask(), page.phyPage->ptr, GetPageSize() );
}

// [dstAddr, dstAddr+size) を含むページに同上
void VirtualMemory::AssignAndCopyPhysicalMemory( u64 addr, u64 size, VIRTUAL_MEMORY_ATTR_TYPE attr )
{
    BlockArray blocks;
    SplitAtMapUnitBoundary(addr, size, back_inserter(blocks) );
    for (BlockArray::iterator e = blocks.begin(); e != blocks.end(); ++e){
        AssignAndCopyPhysicalMemory(e->addr, attr);
    }
}

// Write a page attribute to a page including 'addr'.
void VirtualMemory::SetPageAttribute(  u64 addr, VIRTUAL_MEMORY_ATTR_TYPE attr )
{
    PageTableEntry page;
    if( !m_pageTbl.GetMap( addr, &page ) ) {
        THROW_RUNTIME_ERROR( "The specified address is not mapped." );
    }

    page.attr = attr;
    m_pageTbl.SetMap( addr, page );
}


// Call this method before data is written to memory.
bool VirtualMemory::CopyPageOnWrite( u64 addr )
{
    PageTableEntry page;
    if( !m_pageTbl.GetMap( addr, &page ) ){
        THROW_RUNTIME_ERROR( "The specified address is not mapped." );
        return false;   // To avoid compiler's warning "Uninitialized 'page' is used".
    }

    // A page with reference count that is greater than 1 is a copy and
    // copy-on-write must be done to such a page.
    if( page.phyPage->refCount > 1 ){
        // Copy-on-write
        AssignAndCopyPhysicalMemory( addr, page.attr );
        return true;
    }
    return false;
}
