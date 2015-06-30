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

#include "Sim/ExecUnit/MemExecUnit.h"

#include "Sim/Thread/Thread.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/Memory/Cache/Cache.h"
#include "Sim/Memory/Cache/CacheSystem.h"
#include "Sim/Memory/MemOrderManager/MemOrderManager.h"
#include "Sim/Core/Core.h"
#include "Sim/Op/Op.h"
#include "Sim/Pipeline/Pipeline.h"
#include "Sim/ExecUnit/ExecLatencyInfo.h"
#include "Sim/Foundation/Hook/Hook.h"
#include "Sim/InorderList/InorderList.h"
#include "Sim/Recoverer/Recoverer.h"


using namespace std;
using namespace Onikiri;

MemExecUnit::MemExecUnit() :
    m_cacheSystem(0),
    m_cache(0),
    m_cacheCount(0),
    m_floatConversionLatency( 0 )
{
}

MemExecUnit::~MemExecUnit()
{
    ReleaseParam();
}

void MemExecUnit::Initialize(InitPhase phase)
{
    if(phase == INIT_PRE_CONNECTION){
    }
    else if(phase == INIT_POST_CONNECTION){
        CheckNodeInitialized( "memOrderManager", m_memOrderManager );
        CheckNodeInitialized( "cacheSystem", m_cacheSystem );

        // キャッシュの数を数える
        m_cache = m_cacheSystem->GetFirstLevelDataCache();
        m_cacheCount = 0;
        Cache* cache = m_cache;
        for(; cache != 0; cache = cache->GetNextCache(), ++m_cacheCount) {}
    }
    
    PipelinedExecUnit::Initialize(phase);

}


// 実行レイテンシ後に FinishEvent を登録する
void MemExecUnit::Execute( OpIterator op )
{
    ExecUnitBase::Execute( op );    // Not PipelinedExecUnit
    RegisterEvents( op, GetExecutedLatency( op ) );
}

// Get the actual latency of executed 'op'.
int MemExecUnit::GetExecutedLatency( OpIterator op )
{
    int latency = 0;

    if(op->GetOpClass().IsLoad()) {
        latency = GetExecutedReadLatency(op);
    } else if(op->GetOpClass().IsStore()) {
        latency = GetExecutedWriteLatency(op);
    } else {
        THROW_RUNTIME_ERROR("Unknwon opclass.");
    }

    return latency;
}


// Read の実行レイテンシを返す
// float の場合には、ISA によって規定される変換レイテンシを加えたものを返す
// たとえばalpha21264などでは float の load の変換に1サイクルかかる
int MemExecUnit::GetExecutedReadLatency( OpIterator op )
{
    int readLatency = 0;

    // まず StoreBuffer にアドレスが一致する先行 Store がいないか聞く
    // いない場合は cache にアクセス
    OpIterator producer = GetProducerStore( op );

    // Only the first access is set to op, because the second or later cache 
    // accesses are always partial hit and they are not useful for statistics.
    bool firstAccess = 
        op->GetCacheAccessResult().state == CacheAccessResult::ST_NOT_ACCESSED;

    if( !producer.IsNull() ) {
        readLatency = GetLatency( op->GetOpClass(), 0 );

        if( firstAccess ){
            CacheAccessResult result( 0, CacheAccessResult::ST_HIT, NULL );
            op->SetCacheAccessResult( result );
        }
    } 
    else {
        CacheAccess access( op->GetMemAccess(), op, CacheAccess::OT_READ );
        CacheAccessResult result = m_cache->Read( access, NULL );
        readLatency = result.latency;
        
        if( firstAccess ){
            op->SetCacheAccessResult( result );
        }

        // float の Load だったら変換レイテンシを加える
        if( op->GetOpClass().IsFloat() ) {
            readLatency += m_floatConversionLatency;
        }
    }

    return readLatency;
}

// Writeの実行レイテンシを返す
int MemExecUnit::GetExecutedWriteLatency(OpIterator op)
{
    // StoreBuffer への Write レイテンシ( ISA で規定)を返す
    int code = op->GetOpClass().GetCode();
    int writeLatency = m_execLatencyInfo->GetLatency(code);

    return writeLatency; 
}

// OpCode から取りうるレイテンシの種類の数を返す
int MemExecUnit::GetLatencyCount(const OpClass& opClass)
{
    ASSERT(opClass.IsMem(), "not mem op");
    if( opClass.IsStore() ) {
        // store は固定レイテンシ
        return 1;
    }

    // load はキャッシュの数だけ取りうる可能性がある
    return m_cacheCount;
}

// OpCode とインデクスからレイテンシを返す
int MemExecUnit::GetLatency(const OpClass& opClass, int index)
{
    ASSERT( opClass.IsMem(), "not mem op");
    if( opClass.IsStore() ) {
        // store のレイテンシは ExecLatencyInfo に聞く
        return m_execLatencyInfo->GetLatency(opClass.GetCode());
    }
    
    int latency = 0;
    Cache* cache = m_cache;
    for(int i = 0; i <= index; ++i, cache = cache->GetNextCache()) {
        ASSERT(cache != 0, "cache not set.(index: %d)", index);
        latency += cache->GetStaticLatency();
    }
    
    if( opClass.IsFloat() && opClass.IsLoad() ) {
        latency += m_floatConversionLatency;
    }

    return latency;
}

// Get a producer store of 'consumer'.
OpIterator MemExecUnit::GetProducerStore( OpIterator consumer )
{
    MemOrderManager* memOrderManager = 
        consumer->GetThread()->GetMemOrderManager();

    return
        memOrderManager->GetProducerStore(
            consumer,
            consumer->GetMemAccess()
        );
}
