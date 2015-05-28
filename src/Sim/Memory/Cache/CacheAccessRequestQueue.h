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


#ifndef SIM_MEMORY_CACHE_CACHE_ACCESS_REQUEST_QUEUE_H
#define SIM_MEMORY_CACHE_CACHE_ACCESS_REQUEST_QUEUE_H

#include "Sim/Memory/Cache/CacheTypes.h"

namespace Onikiri
{
    class Pipeline;

    //
    // A queue of cache access requests.
    // This queue is for band width simulation of caches/memories.
    //
    class CacheAccessRequestQueue
    {

    public:
        typedef CacheAccess          Access;
        typedef CacheAccessResult    Result;
        typedef CacheAccessEventType EventType;

        // An access state structure.
        // See comments in the 'Push' method.
        struct AccessState
        {
            Access access;

            u64  id;                    // serial ID of an access
            s64  startTime;             // access start time
            s64  endTime;               // access end time
            s64  serializingStartTime;  // serializing start time
            s64  serializingEndTime;    // serializing end time

            // A notifiee when an access is finished.
            CacheAccessNotifieeIF* notifiee;
            CacheAccessNotificationParam notification;
        };

        typedef pool_list< AccessState > AccessQueue;
        typedef AccessQueue::iterator    AccessQueueIterator;

        CacheAccessRequestQueue( 
            Pipeline* pipe,
            int ports,
            int serializedCycles
        );

        virtual ~CacheAccessRequestQueue();

        // Returns the size of the access list.
        size_t GetSize() const;

        // Push an access to a queue.
        // This method returns the latency of an access.
        // See more detailed comments in the method definition.
        s64 Push( 
            const Access& access, 
            int m_minLatency,
            CacheAccessNotifieeIF* notifiee,
            const CacheAccessNotificationParam& notification
        );

        void Pop( const CacheAccess& access, AccessQueueIterator target );

        void SetEnabled( bool enabled );

    protected:
        
        bool m_enabled;
        u64 m_currentAccessID;

        // Access queue
        AccessQueue m_queue;

        // A pipeline for a connected cache/memory.
        Pipeline* m_pipe;

        // The number of ports of a cache.
        int m_ports;

        // The number of cycles for each access that uses a cache port exclusively.
        int m_serializedCycles;
    
        // Get a time when a new memory access can start.
        s64 GetNextAccessStartableTime();

        // Add an access state to the list.
        void PushAccess(
            const CacheAccess& access, 
            const AccessState& state, 
            int latency 
        );
    };

}; // namespace Onikiri

#endif // SIM_MEMORY_CACHE_CACHE_ACCESS_REQUEST_QUEUE_H

