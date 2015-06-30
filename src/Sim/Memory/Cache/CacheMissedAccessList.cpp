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
// This class handles missed accesses.
// It corresponds to Miss Status Handling Register(MSHR)
//

#include <pch.h>

#include "Sim/Memory/Cache/CacheMissedAccessList.h"
#include "Sim/Memory/Cache/MemoryAccessEndEvent.h"
#include "Sim/Pipeline/Pipeline.h"

using namespace Onikiri;
using namespace std;

CacheMissedAccessList::CacheMissedAccessList(
    Pipeline* pipe,
    int offsetBitSize
) :
    m_enabled( false ),
    m_currentAccessID( 0 ),
    m_pipe( pipe ),
    m_offsetBitSize( offsetBitSize )
{
}

CacheMissedAccessList::~CacheMissedAccessList()
{
}

void CacheMissedAccessList::SetEnabled( bool enabled )
{
    m_enabled = enabled;
}

// Mask the line offset part of 'addr'.
Addr CacheMissedAccessList::MaskLineOffset( Addr addr )
{
    addr.address &= 
        shttl::mask( 
            m_offsetBitSize, 
            64 - m_offsetBitSize 
        );
    return addr;
}

// Find an access corresponding to 'addr' from the list.
// This method returns an iterator of a found access.
CacheMissedAccessList::AccessListIterator 
CacheMissedAccessList::FindAccess( const Addr& addr )
{
    AccessListIterator i = m_list.begin();
    Addr maskedAddr = MaskLineOffset( addr );
    while( i != m_list.end() && i->access.address != maskedAddr ){
        ++i;
    }
    return i;
}

// Find an access corresponding to 'addr' from the list.
// This method returns remaining cycles of a found access.
CacheMissedAccessList::Result CacheMissedAccessList::Find( const Addr& addr )
{
    if( !m_enabled ){
        return Result( 0, Result::ST_MISS );
    }

    AccessListIterator e = FindAccess( addr );
    if( e == m_list.end() ){
        return Result( 0, Result::ST_MISS );
    }

    // hit
    int leftCycles = (int)e->endTime - (int)m_pipe->GetNow();
    if( leftCycles < 0 ){
        THROW_RUNTIME_ERROR( "Invalid Pending Access." );
    }

    return Result( leftCycles, Result::ST_HIT );
}

// Add an access state to the list.
void CacheMissedAccessList::AddList( 
    const CacheAccess& access, const AccessState& state, int latency 
){
    m_currentAccessID++;
    m_list.push_back( state );

    if( m_list.size() >= MAX_MISSED_ACCESS_LIST_LIMIT ){
        THROW_RUNTIME_ERROR( "The size of the missed access list exceeds a limit." );
    }

    // An event of memory access end time.
    AccessListIterator stateIterator = m_list.end(); 
    stateIterator--;
    EventPtr evnt(
        MissedAccessRearchEvent::Construct( access, stateIterator, this )
    );
    m_pipe->AddEvent( evnt, latency ); 
}


// Add a missed access to the list.
void CacheMissedAccessList::Add( 
    const Access& access, 
    const Result& result, 
    CacheAccessNotifieeIF* notifiees[],
    int notifieesCount,
    const CacheAccessNotificationParam& notification
){
    ASSERT( !access.IsWirte(), "A write access is pushed to CacheMissedAccessList." );

    if( !m_enabled ){
        // Notify access finish.
        for( int i = 0; i < notifieesCount; i++ ){
            notifiees[i]->AccessFinished( access, notification );
        }
        return;
    }

    Addr addr = access.address;
    int  latency = result.latency;
    s64  now = m_pipe->GetNow();
        
    CacheMissedAccessList::AccessListIterator 
        foundAccess = FindAccess( addr );

    // A link of a link is forbidden.
    if( foundAccess != m_list.end() && !foundAccess->link ){
        // Add a link to the found predecessor access.
        AccessState state = *foundAccess;
        state.notification = notification;
        state.notifieesCount = notifieesCount;
        for( int i = 0; i < notifieesCount; i++ ){
            state.notifiees[i] = notifiees[i];
        }
        state.link = true;
        state.id = m_currentAccessID;
        int latency = (int)(state.endTime - now);
        AddList( access, state, latency );
        
    }
    else{
        AccessState state;
        state.access    = access;
        state.id        = m_currentAccessID;
        state.startTime = now;
        state.endTime   = now + latency;
        state.link      = false;
        state.notifieesCount = notifieesCount;
        for( int i = 0; i < notifieesCount; i++ ){
            state.notifiees[i] = notifiees[i];
        }
        state.notification = notification;
        state.access.address = MaskLineOffset( state.access.address );
        AddList( access, state, latency );
    } 

}

// When missed accesses are reached to this cache, removing the accesses
// from the list and notify this to the cache.
void CacheMissedAccessList::Remove( 
    const CacheAccess& access, AccessListIterator target 
){
    

    ASSERT( m_enabled );

    // Remove from the list.
    CacheAccessNotificationParam notification = target->notification;
    NotifieeArray notifiees = target->notifiees;
    int notifieesCount = target->notifieesCount;
    m_list.erase( target );
    
    // Notify access finish.
    for( int i = 0; i < notifieesCount; ++i ){
        notifiees[i]->AccessFinished( access, notification );
    }
}

// Returns the size of the access list.
size_t CacheMissedAccessList::GetSize() const
{
    return m_list.size();
}
