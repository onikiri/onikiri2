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

#include "Sim/Memory/MemOrderManager/MemOrderOperations.h"

#include "SysDeps/Endian.h"
#include "Utility/RuntimeError.h"

using namespace std;
using namespace Onikiri;

MemOrderOperations::MemOrderOperations() :
    m_targetIsLittleEndian(false),
    m_memoryAlignment(1),
    m_memoryAlignmentMask(~((u64)0))
{
}

void MemOrderOperations::SetAlignment( int alignment )
{
    ASSERT( alignment > 0 );
    m_memoryAlignment = alignment;

    int bits = 0;
    for( u64 a = alignment; a != 0; a >>= 1 ){
        bits++;
    }
    bits--;
    m_memoryAlignmentMask = shttl::mask( bits, 64-bits );
}


//
// store-load forwarding on LS-queue
//

u64 MemOrderOperations::ReadPreviousAccess( 
    const MemAccess& load, const MemAccess& store )
{
    //
    // ** load access area is sure to be inner of store area.
    //
    //      store addr               
    //           |
    // store:    |<----------store-size----------->|
    // load :    |<--sh-->|<--load-size-->|
    //                    |
    //                load-addr       
    //
    // If a target architecture is big endian, 
    // a store value is converted to memory image of the store value (little endian), and 
    // load a part of the converted store value and convert it again.
    //
    //

    u64 storeRegValue = store.value;
    u64 storeMemValue = CorrectEndian( storeRegValue, store.size );

    u64 sh   = 8 * (load.address.address - store.address.address);
    u64 mask = shttl::mask(0, load.size * 8);

    u64 loadMemValue = (storeMemValue >> sh) & mask;
    u64 loadRegValue = CorrectEndian( loadMemValue, load.size );

    return loadRegValue;    
}

u64 MemOrderOperations::MergePartialAccess( const MemAccess& base, const MemAccess& store )
{
    u64 baseAddr   = base.address.address;
    u64 storeAddr  = store.address.address;
    u64 storeValue = CorrectEndian( store.value, store.sign );
    s64 storeSize  = store.size;

    u64 merged = base.value;

    if( baseAddr < storeAddr ){
        // base   <----->
        // store      <------>
        // start      ^
        s64 start = storeAddr - baseAddr;
        for( int i = 0; i < storeSize; ++i ){
            s64 baseOffset  = i + start;
            if( baseOffset >= base.size )
                break;
            merged = 
                shttl::deposit( merged, baseOffset*8, 8, (storeValue >> (i*8)) & 0xff );
        }
    }
    else{
        // base       <----->
        // store  <------>
        // start      ^
        s64 start = baseAddr - storeAddr;
        for( int i = 0; i < storeSize; ++i ){
            s64 baseOffset  = i - start;
            if( baseOffset < 0 )
                continue;
            if( baseOffset >= base.size )
                break;
            merged = 
                shttl::deposit( merged, baseOffset*8, 8, (storeValue >> (i*8)) & 0xff );
        }

    }

    return merged;
}

// Convert endian for store-load forwarding.
u64 MemOrderOperations::CorrectEndian( u64 src, int size )
{
    if( m_targetIsLittleEndian )
        return src; // Do not need conversion.

    switch( size ){
    case 1:
        return (u64)ConvertEndian((u8)src);
    case 2:
        return (u64)ConvertEndian((u16)src);
    case 4:
        return (u64)ConvertEndian((u32)src);
    case 8:
        return (u64)ConvertEndian((u64)src);
    default:
        THROW_RUNTIME_ERROR( "Invalid size." );
        return 0;
    }

}
    
// アクセスしている範囲に重なりがあるかどうか
bool MemOrderOperations::IsOverlapped(const MemAccess& access1, const MemAccess& access2) const
{
    return IsOverlapped(access1.address.address, access1.size, access2.address.address, access2.size);
}

bool MemOrderOperations::IsOverlapped(u64 addr1, int size1, u64 addr2, int size2) const
{
    return
        (addr1 <= addr2 && addr2 < addr1+size1) ||
        (addr2 <= addr1 && addr1 < addr2+size2);
}

bool MemOrderOperations::IsOverlappedInAligned( const MemAccess& access1, const MemAccess& access2 ) const
{
    return IsOverlappedInAligned(access1.address.address, access1.size, access2.address.address, access2.size);
}

bool MemOrderOperations::IsOverlappedInAligned( u64 addr1, int size1, u64 addr2, int size2 ) const
{
    return (addr1 & m_memoryAlignmentMask) == (addr2 & m_memoryAlignmentMask);
}

// Returns whether a 'inner' access is in a 'outer' access or not.
bool MemOrderOperations::IsInnerAccess( const MemAccess& inner, const MemAccess& outer ) const
{
    return IsInnerAccess(inner.address.address, inner.size, outer.address.address, outer.size);
}

bool MemOrderOperations::IsInnerAccess( u64 inner, int innerSize, u64 outer, int outerSize ) const
{
    //             inner   inner+size
    // inner:        <------->
    // outer:     <------------->
    //          outer       outer+size
    return (outer <= inner) && ((inner + innerSize) <= (outer + outerSize));
}
