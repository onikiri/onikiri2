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
// A queue of cache access requests.
// This queue is for band width simulation of caches/memories.
//

#include <pch.h>

#include "Sim/Memory/Cache/CacheAccessRequestQueue.h"
#include "Sim/Memory/Cache/MemoryAccessEndEvent.h"
#include "Sim/Pipeline/Pipeline.h"

using namespace Onikiri;
using namespace std;

CacheAccessRequestQueue::CacheAccessRequestQueue(
    Pipeline* pipe,
    int ports,
    int serializedCycles
) :
    m_enabled( false ),
    m_currentAccessID( 0 ),
    m_pipe( pipe ),
    m_ports( ports ),
    m_serializedCycles( serializedCycles )
{
}

CacheAccessRequestQueue::~CacheAccessRequestQueue()
{
}

void CacheAccessRequestQueue::SetEnabled( bool enabled )
{
    m_enabled = enabled;
}

// Get a time when new memory access can start.
s64 CacheAccessRequestQueue::GetNextAccessStartableTime()
{
    s64 now = m_pipe->GetNow(); 
    if( m_ports == 0 || m_queue.size() == 0 ){
        return now;
    }

    if( (int)m_queue.size() < m_ports ){
        return now;
    }

    AccessQueue::reverse_iterator firstPortAccess = m_queue.rbegin();
    for( int i = 0; i < m_ports - 1; i++ )
    {
        firstPortAccess++;
        ASSERT( firstPortAccess != m_queue.rend() );
    }

    s64 serializingEndTime = firstPortAccess->serializingEndTime;

    //printf( "%I64d - %I64d\n", now, serializingEndTime );

    if( serializingEndTime < now )
        return now;
    else
        return serializingEndTime;
}

// Add an access state to the list.
void CacheAccessRequestQueue::PushAccess( const CacheAccess& access, const AccessState& state, int latency )
{
    m_currentAccessID++;
    m_queue.push_back( state );

    // An event of memory access end time.
    AccessQueueIterator stateIterator = m_queue.end(); 
    stateIterator--;
    EventPtr evnt(
        CacheAccessEndEvent::Construct( access, stateIterator, this )
    );
    m_pipe->AddEvent( evnt, latency ); 
}


// Push an access to a queue.
// This method returns the latency of an access.
// See more detailed comments in the method definition.
s64 CacheAccessRequestQueue::Push( 
    const Access& access,
    int minLatency,
    CacheAccessNotifieeIF* notifiee,
    const CacheAccessNotificationParam& notification
){
    if( !m_enabled || m_ports == 0 || m_serializedCycles == 0 ){
        // Notify access finish.
        if( notifiee ){
            notifiee->AccessFinished( access, notification );
        }
        return minLatency;
    }

    s64 now = m_pipe->GetNow();

    // Each access exclusively use one cache port for S cycles.
    // After S cycles, a next access can start.
    //   L: latency (Cache/@Latency)
    //   S: serialized cycles ()
    //   *: wait for serializing
    //
    //   A0: <--S--><------(L-S)------>
    //   A1:   *****<--S--><------(L-S)------>
    //   A2:    ***********<--S--><------(L-S)------>

    ASSERT( minLatency >= m_serializedCycles, "Minimum latency is shorter than serialized cycles." );

    s64 serializingStartTime = GetNextAccessStartableTime();
    s64 accessEndTime        = serializingStartTime + minLatency;

    AccessState state = 
    {
        access,                                     // addr
        m_currentAccessID,                          // ID
        now,                                        // start
        accessEndTime,                              // end
        serializingStartTime,                       // serializingStart
        serializingStartTime + m_serializedCycles,  // serializingEnd
        notifiee,                                   // notifiee
        notification                                // type
    };
    
    int latency = (int)( accessEndTime - now );
    PushAccess( access, state, latency );
    return latency;
}

// Remove an access from the queue and notify this to a connected cache
// when a memory access finishes. 
void CacheAccessRequestQueue::Pop( 
    const CacheAccess& access, AccessQueueIterator target 
){

    ASSERT( m_enabled );

    // Remove an access request from the queue.
    CacheAccessNotificationParam notification = target->notification;
    CacheAccessNotifieeIF* notifiee = target->notifiee;
    m_queue.erase( target );
    
    // Notify access finish.
    if( notifiee ){
        notifiee->AccessFinished( access, notification );
    }
}

// Returns the size of the access queue.
size_t CacheAccessRequestQueue::GetSize() const
{
    return m_queue.size();
}
