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

#include "Sim/Memory/Prefetcher/StridePrefetcher.h"
#include "Sim/Op/Op.h"

using namespace Onikiri;

//#define STRIDE_PREFETCHER_DEBUG 1

String StridePrefetcher::Stream::ToString() const
{
    String str;

    // Status
    str += "status: ";
    switch( status ){
    default:
    case SS_INVALID:
        str += "INVALID ";
        break;
    case SS_ALLOCATED:
        str += "ALLOCATED ";
        break;
    }

    // Window
    str += "  pc: ";
    str += pc.ToString();
    str += "  addr: ";
    str += addr.ToString();
    str += "  stride: ";
    str += String().format( "%llx", stride );
    
    // Count
    str += "  count: ";
    str += String().format( "%d", count );
    str += "  conf: ";
    str += String().format( "%d", (int)confidence );
    return str;

}


StridePrefetcher::StridePrefetcher()
{
    m_distance = 0;
    m_degree = 0;
    m_streamTableSize = 0;

    m_numPrefetchingStream = 0;
    m_numAvgPrefetchStreamLength = 0.0;
}


StridePrefetcher::~StridePrefetcher()
{
    if( m_numPrefetchingStream ){
        m_numAvgPrefetchStreamLength = 
            (double)m_numPrefetch / (double)m_numPrefetchingStream;
    }

    ReleaseParam();
}


// --- PhysicalResourceNode
void StridePrefetcher::Initialize(InitPhase phase)
{
    PrefetcherBase::Initialize( phase );

    if(phase == INIT_PRE_CONNECTION){
        
        LoadParam();
        m_streamTable.construct( m_streamTableSize );
    }
    else if(phase == INIT_POST_CONNECTION){
    
    }
}


// This method is called by any cache accesses occurred in a connected cache.
void StridePrefetcher::OnCacheAccess( 
    Cache* cache, 
    const CacheAccess& access,
    bool hit
){
    if( !m_enabled ){
        return;
    }

    OpIterator op = access.op;
    if( op.IsNull() ){
        return;
    }

    Addr pc = op->GetPC();
    Addr missAddr = op->GetMemAccess().address;//access.address;
    //missAddr.address = MaskLineOffset( missAddr.address );

#ifdef STRIDE_PREFETCHER_DEBUG
    g_env.Print(
        String( "access\t" ) + 
        "pc: " + pc.ToString() + 
        "  addr: " + missAddr.ToString() + 
        ( hit ? " hit" : " miss" ) + 
        "\n" 
    );
#endif


//  bool streamHit = false;

    for( size_t i = 0; i < m_streamTable.size(); i++ ){
        Stream* stream = &m_streamTable[i];
        if( stream->status == SS_INVALID )
            continue;
        
        if( stream->pc != pc )
            continue;

        m_streamTable.touch( i );
//      streamHit = true;
        stream->count++;

#ifdef STRIDE_PREFETCHER_DEBUG
        g_env.Print( String( "  stream\t" ) + stream->ToString() + "\n" );
#endif

        if( stream->addr.address + stream->stride == missAddr.address ){

            if( stream->confidence.above_threshold() ){

                for( int d = 0; d < m_degree; d++ ){
                    CacheAccess prefetch;
                    prefetch.op = op;
                    prefetch.type = CacheAccess::OT_PREFETCH;

                    // Prefetch 'end' point of a window.
                    prefetch.address = missAddr;
                    prefetch.address.address += 
                        stream->stride * (m_distance + d);

                    PrefetcherBase::Prefetch( prefetch );
                    //m_prefetchTarget->Read( prefetch, NULL );

#ifdef STRIDE_PREFETCHER_DEBUG
                    g_env.Print( 
                        String( "  prefetch\t" ) + 
                        prefetch.address.ToString() + 
                        "\n" 
                    );
#endif
                }

                IncrementPrefetchNum();
                if( stream->status != SS_PREFETCHING ){
                    m_numPrefetchingStream++;
                }
                stream->status = SS_PREFETCHING;    
            }

            stream->confidence.inc();
            stream->streamLength++;
        }
        else{
            stream->confidence.dec();
        }

        stream->stride = 
            (s64)missAddr.address - (s64)stream->addr.address;
        stream->addr.address = missAddr.address;

        if( stream->stride == 0 ){
            stream->status = SS_INVALID;
        }
        return;
    }

    if( hit )
        return;

    // Allocate a new stride stream.
    Stream stream;
    stream.Reset();
    stream.pc   = pc;
    stream.addr = missAddr;
    stream.orig = missAddr;
    stream.stride = m_lineSize;
    stream.status = SS_ALLOCATED;

    int target = (int)m_streamTable.replacement_target();
    m_streamTable[ target ] = stream;
    m_streamTable.touch( target );

#ifdef STRIDE_PREFETCHER_DEBUG
    g_env.Print( 
        String( "  alloc\t" ) + 
        missAddr.ToString() + " " + 
        stream.ToString() + "\n" 
    );
#endif

//  m_numAllocatedStream++;
}

