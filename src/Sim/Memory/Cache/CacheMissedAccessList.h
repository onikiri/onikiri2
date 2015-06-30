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


#ifndef SIM_MEMORY_CACHE_CACHE_MISSED_ACCESS_LIST_H
#define SIM_MEMORY_CACHE_CACHE_MISSED_ACCESS_LIST_H

#include "Sim/Memory/Cache/CacheTypes.h"

namespace Onikiri
{
    class Pipeline;

    //
    // This class handles missed accesses.
    // It corresponds to Miss Status Handling Register(MSHR)
    //
    class CacheMissedAccessList
    {

    public:
        typedef CacheAccess          Access;
        typedef CacheAccessResult    Result;
        typedef CacheAccessEventType EventType;

        static const size_t MAX_MISSED_ACCESS_LIST_LIMIT = 1024;
        static const size_t MAX_NOTIFIEE_COUNT = 4;
        typedef 
            boost::array<CacheAccessNotifieeIF*, MAX_NOTIFIEE_COUNT>
            NotifieeArray;

        // An access state structure.
        // See comments in'Add' method.
        struct AccessState
        {
            Access access;

            u64  id;                    // serial ID of an access
            s64  startTime;             // access start time
            s64  endTime;               // access end time

            // true if this entry is a link to a actual access.
            // A successor access that hits a predecessor access is 
            // linked to the predecessor access.
            // A link access does not consume a port.
            bool link;                  

            // Notifies when missed accesses are finished.
            NotifieeArray notifiees;
            int notifieesCount;
            CacheAccessNotificationParam
                notification;
        };

        typedef pool_list< AccessState > AccessList;
        typedef AccessList::iterator     AccessListIterator;

        CacheMissedAccessList( 
            Pipeline* pipe, 
            int offsetBitSize
        );

        virtual ~CacheMissedAccessList();

        Result Find( const Addr& addr );

        // Returns the size of the access list.
        size_t GetSize() const;

        // Add a missed access to the list.
        void Add( 
            const Access& access, 
            const Result& result,
            CacheAccessNotifieeIF* notifiees[],
            int notifieesCount,
            const CacheAccessNotificationParam& notification
        );

        void Remove( const CacheAccess& access, AccessListIterator target );

        void SetEnabled( bool enabled );

    protected:
        
        bool m_enabled;

        u64 m_currentAccessID;

        // アクセス中のアクセスを管理するリスト
        AccessList m_list;

        // A pipeline for the cache/memory.
        Pipeline* m_pipe;

        // An address offset of a cache line
        int m_offsetBitSize;

        Addr MaskLineOffset( Addr addr );
        AccessListIterator FindAccess( const Addr& addr );

        // Add an access state to the list.
        void AddList( 
            const CacheAccess& access, const AccessState& state, int latency
        );
    };

}; // namespace Onikiri

#endif // SIM_MEMORY_CACHE_CACHE_MISSED_ACCESS_LIST_H
