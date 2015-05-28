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

#include "Sim/Memory/Cache/CacheSystem.h"
#include "Sim/Memory/Cache/Cache.h"
#include "Sim/Thread/Thread.h"
#include "Sim/InorderList/InorderList.h"
#include "Sim/Op/Op.h"


using namespace std;
using namespace Onikiri;

CacheSystem::CacheSystem() :
    m_firstLvDataCache(0),
    m_firstLvInsnCache(0),
    m_mode( SM_SIMULATION ),
    m_numRetiredStoreForwarding(0),
    m_numRetiredLoadAccess(0),
    m_numRetiredStoreAccess(0)
{
}

CacheSystem::~CacheSystem()
{
}

void CacheSystem::Initialize( InitPhase phase )
{
    if(phase == INIT_PRE_CONNECTION){

        LoadParam();

    }
    else if( phase == INIT_POST_CONNECTION ){

        // Initialize cache hierarchy and related information
        InitCacheHierarchy( &m_dataCacheHierarchy, &m_dataCacheHierarchyStr, m_firstLvDataCache );
        InitCacheHierarchy( &m_insnCacheHierarchy, &m_insnCacheHierarchyStr, m_firstLvInsnCache );
        m_loadAccessDispersion.resize( m_dataCacheHierarchy.size(), 0 );
        m_storeAccessDispersion.resize( m_dataCacheHierarchy.size(), 0 );
        
    }
}

void CacheSystem::Finalize()
{
    // For memory accesses
    m_memoryAccessDispersion.clear();
    m_numRetiredMemoryAccess = 0;
    for( size_t i = 0; i < m_dataCacheHierarchy.size(); ++i ){
        m_memoryAccessDispersion.push_back(
            m_loadAccessDispersion[i] + m_storeAccessDispersion[i]
        );
        m_numRetiredMemoryAccess += m_memoryAccessDispersion[i];
    }

    // Calculate coverage of each cache hierarchy.
    CalculateCoverage( &m_loadAccessCoverage,   m_loadAccessDispersion  );
    CalculateCoverage( &m_storeAccessCoverage,  m_storeAccessDispersion );
    CalculateCoverage( &m_memoryAccessCoverage, m_memoryAccessDispersion );

    // Calculate retired instructions.
    s64 retiredInsns = 0;
    for( int i = 0; i < m_thread.GetSize(); i++ ){
        InorderList* ioList = m_thread[i]->GetInorderList();
        if( ioList ){
            retiredInsns += ioList->GetRetiredInsns();
        }
    }

    // Calculate MPKIs
    CalculateMissPerKiloInsns( &m_loadMPKI,  m_loadAccessDispersion,   retiredInsns );
    CalculateMissPerKiloInsns( &m_storeMPKI, m_storeAccessDispersion,  retiredInsns );
    CalculateMissPerKiloInsns( &m_totalMPKI, m_memoryAccessDispersion, retiredInsns );

    ReleaseParam();
}

// Calculate coverage from the number of raw accesses.
void CacheSystem::CalculateCoverage( 
    vector< double >* coverage, 
    const vector< s64 >& dispersion 
){
    s64 total = 0;
    for( vector< s64 >::const_iterator i = dispersion.begin(); i != dispersion.end(); ++i ){
        total += *i;
    }

    s64 covered = 0;
    coverage->clear();
    for( vector< s64 >::const_iterator i = dispersion.begin(); i != dispersion.end(); ++i ){
        covered += *i;
        coverage->push_back( (double)covered / (double)total );
    }
}


// Calculate MPKI from the number of raw accesses.
void CacheSystem::CalculateMissPerKiloInsns(
    std::vector< double >* mpki, 
    const std::vector< s64 >& dispersion,
    s64 retiredInsns
){
    s64 misses = 0;
    for( vector< s64 >::const_iterator i = dispersion.begin(); i != dispersion.end(); ++i ){
        misses += *i;
    }

    mpki->clear();
    for( vector< s64 >::const_iterator i = dispersion.begin(); i != dispersion.end(); ++i ){
        misses -= *i;
        mpki->push_back( 1000.0 * (double)misses / (double)retiredInsns );
    }
}


void CacheSystem::InitCacheHierarchy( 
    vector< Cache* >* hierarchy, 
    vector< string >* hierarchyStr,
    Cache* top 
){
    hierarchy->clear();
    Cache* cache = top;
    for( int i = 0; cache; i++){
        hierarchy->push_back( cache );
        hierarchyStr->push_back( cache->GetName() );
        cache = cache->GetNextCache();
    }
}

int CacheSystem::GetDataCacheLevel( Cache* cache )
{
    for( size_t i = 0; i < m_dataCacheHierarchy.size(); ++i ){
        if( cache == m_dataCacheHierarchy[i] ){
            return (int)i;
        }
    }

    ASSERT( "An unknow cache is passed." );
    return 0;
}


void CacheSystem::Commit( OpIterator op )
{
    const OpClass& opClass = op->GetOpClass();

    if( opClass.IsMem() ){
        // Update dispersion of accesses
        int level = 0;
        const CacheAccessResult& result = op->GetCacheAccessResult();
        if( result.cache == NULL ){ // NULL is store forwarding.
            if( opClass.IsLoad() ){
                m_numRetiredStoreForwarding++;
            }
        }
        else{
            level = GetDataCacheLevel( op->GetCacheAccessResult().cache );
        }

        if( opClass.IsLoad() ){
            ASSERT( level < (int)m_loadAccessDispersion.size() );
            m_loadAccessDispersion[ level ]++;
            m_numRetiredLoadAccess++;
        }
        else{
            ASSERT( level < (int)m_storeAccessDispersion.size() );
            m_storeAccessDispersion[ level ]++;
            m_numRetiredStoreAccess++;
        }
    }
}


// This method is called when a simulation mode is changed.
void CacheSystem::ChangeSimulationMode( PhysicalResourceNode::SimulationMode mode )
{
    m_mode = mode;
}
