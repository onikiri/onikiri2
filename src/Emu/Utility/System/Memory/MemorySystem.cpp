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
#include "Emu/Utility/System/Memory/MemorySystem.h"
#include "Emu/Utility/System/Memory/MemoryUtility.h"
#include "SysDeps/Endian.h"

using namespace std;
using namespace Onikiri;
using namespace EmulatorUtility;

// Enable copy-on-write scheme in the mmap implement.
#define ENABLE_MEMORY_SYSTEM_COPY_ON_WRITE

//
// MemorySystem
//
MemorySystem::MemorySystem( int pid, bool bigEndian, SystemIF* simSystem ) : 
    m_virtualMemory( pid, bigEndian, simSystem ), 
    m_heapAlloc( m_virtualMemory.GetPageSize() ), 
    m_currentBrk(0), 
    m_simSystem(simSystem),
    m_pid(pid),
    m_bigEndian(bigEndian)
{
    u64 pageSize = GetPageSize();

    // メモリ確保時の0初期化ページ
    u64 zeroPageAddr = RESERVED_PAGE_ZERO_FILLED * pageSize;
    m_virtualMemory.AssignPhysicalMemory( zeroPageAddr, VIRTUAL_MEMORY_ATTR_READ ); // Read only    
    m_virtualMemory.TargetMemset( zeroPageAddr, 0, pageSize );  // TargetMemset ignores page attribute.

    if( ( pageSize & (pageSize - 1) ) != 0 ){
        THROW_RUNTIME_ERROR( "The page size is not power-of-two." );
    }
}

MemorySystem::~MemorySystem()
{
    // メモリ確保時の0初期化ページ
    u64 zeroPageAddr = RESERVED_PAGE_ZERO_FILLED * GetPageSize();
    m_virtualMemory.FreePhysicalMemory( zeroPageAddr );
}


//
// メモリ管理
//
u64 MemorySystem::GetPageSize()
{
    return m_heapAlloc.GetPageSize();
}

void MemorySystem::AddHeapBlock(u64 addr, u64 size)
{
    m_heapAlloc.AddMemoryBlock(addr, size);
}

void MemorySystem::SetInitialBrk(u64 initialBrk)
{
    m_currentBrk = initialBrk;
}

static u64 MaskPageOffset( u64 addr, u64 pageSize )
{
    return addr & ~(pageSize - 1);
}

u64 MemorySystem::Brk(u64 addr)
{

    if (addr == 0)
        return m_currentBrk;

    u64 pageSize = GetPageSize();

    
    if( addr < m_currentBrk ){
        // Shrink program break and free unused memory.
        u64 endPageAddr = MaskPageOffset( addr, pageSize );
        u64 pageAddr    = MaskPageOffset( m_currentBrk, pageSize );
        while( pageAddr != endPageAddr ){
            m_virtualMemory.FreePhysicalMemory( pageAddr, pageSize );
            pageAddr -= pageSize;
        }
        m_currentBrk = addr;
        return 0;
    }
    else{
        // Expand program break and allocate memory.
        u64 endPageAddr  = MaskPageOffset( addr, pageSize );
        u64 pageAddr     = MaskPageOffset( m_currentBrk, pageSize ) + pageSize;
        //u64 zeroPageAddr = RESERVED_PAGE_ZERO_FILLED * GetPageSize();

        while( pageAddr <= endPageAddr ){
            //m_virtualMemory.SetPhysicalMemoryMapping( pageAddr, zeroPageAddr );
            m_virtualMemory.AssignPhysicalMemory( pageAddr, VIRTUAL_MEMORY_ATTR_READ|VIRTUAL_MEMORY_ATTR_WRITE );
            pageAddr += pageSize;
        }
        TargetMemset(m_currentBrk, 0, addr-m_currentBrk);
        m_currentBrk = addr;
    }


    return 0;
}


u64 MemorySystem::MMap(u64 addr, u64 length)
{
    if (addr != 0)
        return (u64)-1;
    if (length <= 0)
        return (u64)-1;

    u64 result = m_heapAlloc.Alloc(addr, length);
    CheckValueOnPageBoundary( result, "mmap:address" );

    if (result) {
#ifdef ENABLE_MEMORY_SYSTEM_COPY_ON_WRITE
        u64 zeroPageAddr = RESERVED_PAGE_ZERO_FILLED * GetPageSize();
        m_virtualMemory.SetPhysicalMemoryMapping( result, zeroPageAddr, length, VIRTUAL_MEMORY_ATTR_READ|VIRTUAL_MEMORY_ATTR_WRITE );
#else
        m_virtualMemory.AssignPhysicalMemory(result, length, VIRTUAL_MEMORY_ATTR_READ);
        TargetMemset(result, 0, length);
#endif
        return result;
    }
    else{
        return (u64)-1;
    }
}

u64 MemorySystem::MRemap(u64 old_addr, u64 old_size, u64 new_size, bool mayMove)
{
    // Debug
    //g_env.Print( "MemorySystem::MRemap\n" );

    CheckValueOnPageBoundary( old_addr, "mremap:old_addr" );

    if (new_size <= 0)
        return (u64)-1;
    // そんな領域はない
    if (m_heapAlloc.GetBlockSize(old_addr) != old_size)
        return (u64)-1;

    u64 result = m_heapAlloc.ReAlloc(old_addr, old_size, new_size);
    CheckValueOnPageBoundary( result, "mremap/not move" );

    if (result) {
        CheckValueOnPageBoundary( old_size, "mremap:old_size" );
        CheckValueOnPageBoundary( new_size, "mremap:new_size" );
        if( new_size > old_size ){
            
            u64 pageMask = GetPageSize() - 1;
            if( ( ( result + old_size ) & pageMask) != 0 ){
                // 古い領域の最後尾がページ境界に無かった場合，ページが既に確保済み
                s64 length = (s64)new_size - (s64)(( old_size + pageMask ) & ~pageMask);
                u64 addr   = result + new_size - length;
                if( length > 0 ){
                    m_virtualMemory.AssignPhysicalMemory( addr, length, VIRTUAL_MEMORY_ATTR_READ|VIRTUAL_MEMORY_ATTR_WRITE );
                }
            }
            else{
                m_virtualMemory.AssignPhysicalMemory(result+old_size, new_size - old_size, VIRTUAL_MEMORY_ATTR_READ|VIRTUAL_MEMORY_ATTR_WRITE );
            }
            TargetMemset(result+old_size, 0, new_size-old_size);
        }
        else if( new_size < old_size ){
            u64 pageMask = GetPageSize() - 1;
            if( ( ( result + new_size ) & pageMask ) != 0 ){
                // 新しい領域の最後尾がページ境界に無かった場合，ページを解放しちゃ駄目
                // Todo: 古い領域の最後尾が境界に無かった場合？
                s64 length = (s64)old_size - (s64)(( new_size + pageMask ) & ~pageMask);
                u64 addr   = result + old_size - length;
                if( length > 0 ){
                    m_virtualMemory.FreePhysicalMemory( addr, length );
                }
            }
            else{
                m_virtualMemory.FreePhysicalMemory(result+new_size, old_size - new_size);
            }
        }

        return result;
    }
    else if (mayMove) {
        // 別の位置に領域を確保
        ASSERT(old_size < new_size, "HeapAlloc::ReAlloc Logic Error");
        result = m_heapAlloc.Alloc(0, new_size);
        CheckValueOnPageBoundary( result, "mremap/move" );

        if (result) {
            m_virtualMemory.AssignPhysicalMemory(result, new_size, VIRTUAL_MEMORY_ATTR_READ|VIRTUAL_MEMORY_ATTR_WRITE );

            // 元の領域のデータを新しい領域にコピー
            // Note: The destructor of TargetBuffer must be called before memory pages are freed.
            {
                TargetBuffer buf(this, old_addr, static_cast<size_t>(old_size));
                MemCopyToTarget(result, buf.Get(), old_size);
                TargetMemset(result+old_size, 0, new_size-old_size);
            }
            
            MUnmap(old_addr, old_size);
            return result;
        }
        else {
            return (u64)-1;
        }
    }
    else {
        return (u64)-1;
    }
}

int MemorySystem::MUnmap(u64 addr, u64 length)
{
    CheckValueOnPageBoundary( addr,   "munmap:address" );
    CheckValueOnPageBoundary( length, "munmap:length" );

    if (length <= 0)
        return -1;

    if (m_heapAlloc.Free(addr, length)) {
        // 領域の最後尾がページ境界に無かった場合，ページを解放しちゃ駄目
        u64 pageMask = GetPageSize() - 1;
        if( ( ( addr + length ) & pageMask ) != 0 ){
            length = length & ~pageMask;
        }
        m_virtualMemory.FreePhysicalMemory( addr, length );
        return 0;
    }
    else
        return -1;
}

// Check whether an address is aligned on a page boundary.
// An address passed to munmap/mremap must be aligned to a page boundary.
void MemorySystem::CheckValueOnPageBoundary( u64 addr, const char* signature  )
{
    if( ( addr & (GetPageSize() - 1) ) != 0 ){
        RUNTIME_WARNING( 
            "The address is not aligned to a page boundary in '%s'\n"
            "Address : %08x%08x",
            (u32)(addr >> 32), (u32)addr,
            signature
        );
    }
}

