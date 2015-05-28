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
#include "Sim/Memory/Prefetcher/StreamPrefetcher.h"
#include "Sim/Dumper/Dumper.h"


using namespace Onikiri;

//#define STREAM_PREFETCHER_DEBUG 1

String StreamPrefetcher::Stream::ToString() const
{
    String str;

    // Status
    str += "status: ";
    switch( status ){
    default:
    case SS_INVALID:
        str += "INVALID ";
        break;
    case SS_TRAINING:
        str += "TRAINING";
        break;
    case SS_MONITOR:
        str += "MONITOR ";
        break;
    }

    // Window
    str += "  addr: ";
    str += String().format( "%016llx", addr.address );
    str += "  dir: ";
    str += ascending ? "a" : "d";
    str += "  orig: ";
    str += String().format( "%016llx", orig.address );

    // Count
    str += "  count: ";
    str += String().format( "%d", count );
    return str;
}


StreamPrefetcher::StreamPrefetcher()
{
    m_distance = 0;
    m_degree = 0;
    m_trainingWindowSize = 0;
    m_trainingThreshold = 0;
    m_streamTableSize = 0;
    m_effectiveDistance = 0;
    m_effectiveTrainingWindow = 0;

    m_numMonitioredStream = 0;
    m_numAllocatedStream = 0;
    m_numTrainingAccess = 0;
    m_numAlocatingAccess = 0;
                
    m_numAscendingMonitioredStream = 0;
    m_numDesscendingMonitioredStream = 0;
    
    m_numAvgMonitoredStreamLength = 0.0;
}


StreamPrefetcher::~StreamPrefetcher()
{
    if( m_numMonitioredStream ){
        m_numAvgMonitoredStreamLength = 
            (double)m_numPrefetch / (double)m_numMonitioredStream;
    }

    ReleaseParam();
}


// --- PhysicalResourceNode
void StreamPrefetcher::Initialize(InitPhase phase)
{
    PrefetcherBase::Initialize( phase );

    if(phase == INIT_PRE_CONNECTION){
        
        LoadParam();

        // Setup remaining members from loaded parameters.
        m_streamTable.construct( m_streamTableSize );
        m_effectiveDistance = m_distance * m_lineSize;
        m_effectiveTrainingWindow = m_lineSize * m_trainingWindowSize;

    }
    else if(phase == INIT_POST_CONNECTION){
    
    }
}


// Returns whether the 'addr' is in a window specified 
// by the remaining arguments or not. The 'ascending' means
// a direction of a window. 
bool StreamPrefetcher::IsInWindow( u64 addr, u64 start, u64 windowSize, bool ascending )
{
    if( ascending ){
        return ( start <= addr && addr < start + windowSize );
    }
    else{
        return ( start - windowSize <= addr && addr < start );
    }
}


bool StreamPrefetcher::IsInWindow( const Addr& addr, const Addr& start, u64 windowSize, bool ascending )
{
    if( addr.pid != start.pid )
        return false;
    
    u64 rawAddr  = MaskLineOffset( addr.address  );
    u64 rawStart = MaskLineOffset( start.address );
    return IsInWindow( rawAddr, rawStart, windowSize, ascending );
}


// Do prefetch an address specified by the 'stream' and update the 'stream'. 
void StreamPrefetcher::Prefetch( const CacheAccess& kickerAccess,Stream* stream )
{
    for( int i = 0; i < m_degree; i++ ){

        CacheAccess prefetch;
        prefetch.op = kickerAccess.op;
        prefetch.type = CacheAccess::OT_PREFETCH;
        
        // Prefetch 'end' point of a window.
        prefetch.address = stream->addr;
        if( stream->ascending )
            prefetch.address.address += m_effectiveDistance;
        else
            prefetch.address.address -= m_effectiveDistance;

        PrefetcherBase::Prefetch( prefetch );
        //m_prefetchTarget->Read( prefetch, NULL );

#ifdef STREAM_PREFETCHER_DEBUG
        g_env.Print( ( String( "  prefetch\t" ) + prefetch.address.ToString() + "\n" ).c_str() );
#endif

        // Update a stream
        stream->addr.address += 
            stream->ascending ? m_lineSize : -m_lineSize;

    }

    IncrementPrefetchNum();
}


// Check and update entries with MONITOR status in the stream table.
// Returns whether any entries are updated or not.
bool StreamPrefetcher::UpdateMonitorStream( const CacheAccess& access )
{
    Addr missAddr = access.address;
    missAddr.address = MaskLineOffset( missAddr.address );

    // Monitor
    for( size_t i = 0; i < m_streamTable.size(); i++ ){
        Stream* stream = &m_streamTable[i];
        if( stream->status != SS_MONITOR )
            continue;

        // Check a missed address is in a prefetch window.
        if( !IsInWindow( missAddr, stream->addr, m_effectiveDistance, stream->ascending ) ){
            continue;
        }

#ifdef STREAM_PREFETCHER_DEBUG
        g_env.Print( 
            ( String( "  monitor\t" ) + stream->ToString() + "\n" ).c_str() 
            );
#endif

        // Prefetch
        // Note: The 'Prefetch' method may update a entry in the stream table.
        Prefetch( access, stream );

        // Update the table
        m_streamTable.touch( i );
        stream->count++;

        return true; // A prefetch process is finished.
    }

    return false;
}


// Check and update entries with SS_TRAINING status in the stream table.
bool StreamPrefetcher::UpdateTrainingStream( const CacheAccess& access )
{
    Addr missAddr = access.address;
    missAddr.address = MaskLineOffset( missAddr.address );

    for( size_t i = 0; i < m_streamTable.size(); i++ ){
        Stream* stream = &m_streamTable[i];
        if( stream->status != SS_TRAINING )
            continue;

        u64 window = m_effectiveTrainingWindow;
        const Addr& start = stream->orig;

        // Check a missed address is in a training window.
        bool inWindow  = false;
        bool ascending = false;
        if( IsInWindow( missAddr, start, window, true ) ){
            inWindow  = true;
            ascending = true;
        }
        else if( IsInWindow( missAddr, start, window, false ) ){
            inWindow  = true;
            ascending = false;
        }

        if( !inWindow )
            continue;

#ifdef STREAM_PREFETCHER_DEBUG
        g_env.Print( ( String( "  train\t" ) + missAddr.ToString() + " " + stream->ToString() + "\n" ).c_str() );
#endif

        // Decide a stream direction of a stream when 
        // an access is a first access (stream->count == 0)
        // in a training mode.
        if( stream->count == 0 ){
            stream->ascending = ascending;
        }

        if( stream->ascending == ascending ){
            // Update the training count.
            stream->count++;
        }
        else{
            // Reset a counter and a direction flag.
            stream->ascending = ascending;
            stream->count = 0;
            continue;   // A prefetch process is continued.
        }

        // State transition to MONITOR status.
        if( stream->count >= m_trainingThreshold ){
            stream->status = SS_MONITOR;
            m_numMonitioredStream++;
            if( ascending )
                m_numAscendingMonitioredStream++;
            else
                m_numDesscendingMonitioredStream++;

#ifdef STREAM_PREFETCHER_DEBUG
            g_env.Print( ( String( "  to monitor\t" ) + missAddr.ToString() + " " + stream->ToString() + "\n" ).c_str() );
#endif
        }

        // Training
        m_numTrainingAccess++;
        m_streamTable.touch( i );
        //stream->addr = missAddr;

        return true; // A prefetch process is finished.
    }

    return false;
}


// Allocate a new entry in the stream table.
void StreamPrefetcher::AllocateStream( const CacheAccess& access )
{
    Addr missAddr = access.address;
    missAddr.address = MaskLineOffset( missAddr.address );

    Stream stream;
    stream.addr = missAddr;
    stream.orig = missAddr;
    stream.status = SS_TRAINING;
    stream.count = 0;

    int target = (int)m_streamTable.replacement_target();
    m_streamTable[ target ] = stream;
    m_streamTable.touch( target );
    m_numAlocatingAccess++;
    m_numAllocatedStream++;

#ifdef STREAM_PREFETCHER_DEBUG
    g_env.Print( ( String( "  alloc\t" ) + missAddr.ToString() + " " + stream.ToString() + "\n" ).c_str() );
#endif
}


// This method is called by any cache accesses occurred in a connected cache.
void StreamPrefetcher::OnCacheAccess( 
    Cache* cache, 
    const CacheAccess& access,
    bool hit
){
    if( !m_enabled ){
        return;
    }

#ifdef STREAM_PREFETCHER_DEBUG
    g_env.Print( ( String( "access\t" ) + access.address.ToString() + "\n" ).c_str() );
#endif

    bool streamTableHit = false;

    // Update monitor streams.
    if( UpdateMonitorStream( access ) ){
        streamTableHit = true;
    }

    // Training and allocation are done by a cache missed access only.
    if( hit ){
        return;
    }

    // Update training streams.
    if( UpdateTrainingStream( access ) ){   
        streamTableHit = true;
    }

    // Allocate a new stream.
    if( !streamTableHit ){
        AllocateStream( access );
    }
}

