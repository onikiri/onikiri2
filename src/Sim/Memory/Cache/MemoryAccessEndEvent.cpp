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

#include "Sim/Memory/Cache/MemoryAccessEndEvent.h"


using namespace Onikiri;

CacheAccessEndEvent::CacheAccessEndEvent( 
    const CacheAccess& access, 
    AccessQueueIterator target, 
    CacheAccessRequestQueue* accessReqQueue 
) :
    m_access        ( access ),
    m_target        ( target ),
    m_accessReqQueue( accessReqQueue )
{
}

// m_addrのPendingAccessが終了したのでPendingAccessから除外する
void CacheAccessEndEvent::Update()
{
    m_accessReqQueue->Pop( m_access, m_target );
}



MissedAccessRearchEvent::MissedAccessRearchEvent( 
    const CacheAccess& access,
    CacheMissedAccessList::AccessListIterator target, 
    CacheMissedAccessList* accessList 
) :
    m_access        ( access ),
    m_target        ( target ),
    m_pendingAccess ( accessList )
{
}

// m_addrのPendingAccessが終了したのでPendingAccessから除外する
void MissedAccessRearchEvent::Update()
{
    m_pendingAccess->Remove( m_access, m_target );
}
