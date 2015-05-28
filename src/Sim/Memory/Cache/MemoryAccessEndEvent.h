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


#ifndef SIM_MEMORY_CACHE_MEMORY_ACCESS_END_EVENT_H
#define SIM_MEMORY_CACHE_MEMORY_ACCESS_END_EVENT_H

#include "Sim/Foundation/Event/EventBase.h"
#include "Sim/Op/OpArray/OpArray.h"

#include "Sim/Memory/Cache/CacheMissedAccessList.h"
#include "Sim/Memory/Cache/CacheAccessRequestQueue.h"

namespace Onikiri
{
    // An event for notifying the end of cache accesses to CacheAccessRequestQueue.
    class CacheAccessEndEvent :
        public EventBase<CacheAccessEndEvent>
    {
    public:

        typedef
            CacheAccessRequestQueue::AccessQueueIterator
            AccessQueueIterator;

        CacheAccessEndEvent( 
            const CacheAccess& access, 
            AccessQueueIterator target, 
            CacheAccessRequestQueue* accessReqQueue 
        );

        virtual void Update(); 

    private:
        CacheAccess         m_access;       
        AccessQueueIterator m_target;
        CacheAccessRequestQueue*    m_accessReqQueue;
    };



    // PendingAccessの終了タイミングをキャッシュに伝えるためのクラス
    // 他のEventと異なり、opがflushされてもキャッシュアクセスはキャンセルされないので、
    // このEventがキャンセルされることはない
    // そのためopのm_eventにはMemoryAccessEndEventは追加せず、Cacheの中でこのEventを管理する
    class MissedAccessRearchEvent :
        public EventBase<MissedAccessRearchEvent>
    {
    private:
        // PendingAccessを開始したアドレス
        CacheAccess         m_access;       
        CacheMissedAccessList::AccessListIterator
                            m_target;
        CacheMissedAccessList*  m_pendingAccess;

    public:
        MissedAccessRearchEvent( 
            const CacheAccess& access, 
            CacheMissedAccessList::AccessListIterator target, 
            CacheMissedAccessList* accessList 
        );

        // m_addrのPendingAccessが終了することをキャッシュに通知
        virtual void Update(); 
    };

}; // namespace Onikiri

#endif // SIM_MEMORY_CACHE_MEMORY_ACCESS_END_EVENT_H

