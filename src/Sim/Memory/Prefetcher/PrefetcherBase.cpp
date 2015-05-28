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

#include "Sim/Memory/Prefetcher/PrefetcherBase.h"
#include "Sim/Memory/Cache/Cache.h"

using namespace Onikiri;

PrefetcherBase::PrefetcherBase()
{
    m_prefetchTarget = NULL;

    m_enabled = false;

    m_lineSize = 0;
    m_lineBitSize = 0;

    m_numPrefetch = 0;
    m_numReadHitAccess = 0;
    m_numReadMissAccess = 0;
    m_numWriteHitAccess = 0;
    m_numWriteMissAccess = 0;

    m_numReplacedLine = 0;
    m_numPrefetchedReplacedLine = 0;
    m_numEffectivePrefetchedReplacedLine = 0;

    m_mode = SM_EMULATION;
    
}

PrefetcherBase::~PrefetcherBase()
{
}

// --- PhysicalResourceNode
void PrefetcherBase::Initialize(InitPhase phase)
{
    if(phase == INIT_PRE_CONNECTION){

        LoadParam();

        m_lineSize = 1 << m_lineBitSize;
    }
    else if(phase == INIT_POST_CONNECTION){
        m_exLineState.Resize( m_prefetchTarget );
    }
}

void PrefetcherBase::ChangeSimulationMode( SimulationMode mode )
{
    m_mode = mode;
}

// Update cache access statistics.
void PrefetcherBase::UpdateCacheAccessStat( const CacheAccess& access, bool hit )
{
    switch( access.type ){
    case CacheAccess::OT_READ:
    case CacheAccess::OT_PREFETCH:
    case CacheAccess::OT_READ_FOR_WRITE_ALLOCATE:
        if( hit )
            m_numReadHitAccess++;
        else
            m_numReadMissAccess++;
        break;
    case CacheAccess::OT_WRITE:
    case CacheAccess::OT_WRITE_BACK:
        if( hit )
            m_numWriteHitAccess++;
        else
            m_numWriteMissAccess++;
        break;
    default:
        THROW_RUNTIME_ERROR( "Invalid access." );
        break;
    }
}

// Increment the number of prefetch.
void PrefetcherBase::IncrementPrefetchNum()
{
    m_numPrefetch++;
}

// Masks offset bits of the cache line in the 'addr'.
u64 PrefetcherBase::MaskLineOffset( u64 addr )
{
    return addr & ~((1 << m_lineBitSize) - 1);
}

// Returns whether an access is prefetch or not.
bool PrefetcherBase::IsPrefetch( const CacheAccess& access )
{
    return access.type == CacheAccess::OT_PREFETCH;
}


// Find a prefetching access with same the access's address. 
PrefetcherBase::AccessList::iterator 
PrefetcherBase::FindPrefetching( const Addr& address )
{
    for( pool_list< PrefetchAccess >::iterator i = m_accessList.begin();
        i != m_accessList.end();
        ++i
    ){
        if( address == i->address){
            return i;
        }
    }

    return m_accessList.end();
}


// Cache read
void PrefetcherBase::OnCacheRead( Cache* cache, CacheHookParam* param )
{
    if( !m_enabled )
        return;

    if( !IsPrefetch( *param->access ) || !IsAccessFromThisPrefetcher( param ) ){
        bool hit = param->result.state != CacheAccessResult::ST_MISS;
        OnCacheAccess( 
            cache, 
            *param->access, 
            hit
        );

        if( param->line != param->table->end() ){
            ExLineState* state = &m_exLineState[ param->line ];
            state->accessed = true;
        }

        ASSERT( param->access );
        AccessList::iterator prefetch = FindPrefetching( param->access->address );
        if( prefetch != m_accessList.end() ){
            prefetch->accessdCount++;
        }

        // Update statistics about cache accesses.
        UpdateCacheAccessStat( *param->access, hit );
    }
}

// Cache write
void PrefetcherBase::OnCacheWrite( Cache* cache, CacheHookParam* param )
{
    if( !m_enabled )
        return;

    if( !IsPrefetch( *param->access ) || !IsAccessFromThisPrefetcher( param ) ){
        bool hit = param->result.state != CacheAccessResult::ST_MISS;
        OnCacheAccess( 
            cache, 
            *param->access, 
            hit
        );

        // Update statistics about cache accesses.
        UpdateCacheAccessStat( *param->access, hit );

        if( param->line != param->table->end() ){
            ExLineState* state = &m_exLineState[ param->line ];
            state->accessed = true;
        }
    }
}

// Invalidate line
void PrefetcherBase::OnCacheInvalidation( Cache* cache, CacheHookParam* param )
{
    if( !m_enabled )
        return;

    if( param->line != param->table->end() ){
        m_exLineState[ param->line ].valid = false;
    }
    
    AccessList::iterator prefetch = FindPrefetching( *param->address );
    if( prefetch != m_accessList.end() ){
        prefetch->invalidated = true;
    }
}

// Write an access to the cache table.
void PrefetcherBase::OnCacheTableUpdate( Cache* cache, CacheHookParam* param )
{
    if( !m_enabled )
        return;

    ExLineState* state = &m_exLineState[ param->line ];

    if( param->replaced ){
        m_numReplacedLine++;

        if( state->valid && state->prefetched ){
            m_numPrefetchedReplacedLine++;
            if( state->accessed ){
                m_numEffectivePrefetchedReplacedLine++;
            }
        }

    }


    state->valid = true;
    if( IsPrefetch( *param->access ) ){
        state->prefetched = true;
        state->accessed   = false;
        ASSERT( param->access );
        AccessList::iterator prefetch = FindPrefetching( param->access->address );
        if( prefetch != m_accessList.end() && prefetch->accessdCount > 0 ){
            state->accessed = true;
        }
    }
    else{
        state->prefetched = false;
        state->accessed   = true;
    }
}

// Invoke a prefetch access.
void PrefetcherBase::Prefetch( const CacheAccess& access )
{
    CacheAccess prefetch = access;
    prefetch.type = CacheAccess::OT_PREFETCH;
    prefetch.address.address = MaskLineOffset( access.address.address );

    
    AccessList::iterator current;
    if( m_mode == SM_SIMULATION ){ 
        current = m_accessList.insert( m_accessList.end(), prefetch );
        if( m_accessList.size() >= MAX_INFLIGHT_PREFETCH_ACCESSES ){
            THROW_RUNTIME_ERROR( "The size of the prefetch access list exceeds a limit." );
        }
    }

    CacheAccessResult result =
        m_prefetchTarget->Read( prefetch, this );

    if( m_mode == SM_SIMULATION ){ 
        if( result.state != CacheAccessResult::ST_MISS ){
            m_accessList.erase( current );
        }
    }
}

void PrefetcherBase::AccessFinished( 
    const CacheAccess& access, 
    const CacheAccessNotificationParam& param 
){
    if( m_mode != SM_SIMULATION ){ 
        return;
    }

    ASSERT( 
        param.result.state != CacheAccessResult::ST_NOT_ACCESSED,
        "Not initialized notification is reached."
    );

    switch( param.type ){

    case CAET_FILL_FROM_NEXT_CACHE_FINISHED:
    case CAET_FILL_FROM_MAL_FINISHED:
    case CAET_READ_ACCESS_FINISHED:

        if( param.result.state == CacheAccessResult::ST_MISS ){
            AccessList::iterator prefetch = FindPrefetching( access.address );
            if( prefetch != m_accessList.end() ){
                m_accessList.erase( prefetch );
            }
            else{
                ASSERT( 0, "An unknown prefetch access is finished.")
            }
        }
        break;
    
    default:
        ASSERT( 0, "A write access must not be notified.." );
        break;
    }
}

// Returns whether this access is from this prefetcher or not.
bool PrefetcherBase::IsAccessFromThisPrefetcher( CacheHookParam* param ) const
{
    if( !param->notifiee ){
        return false;
    }

    return param->notifiee == static_cast<const CacheAccessNotifieeIF*>(this);
}

