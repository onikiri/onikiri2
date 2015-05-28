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

#include "Sim/Core/Core.h"

#include "Utility/RuntimeError.h"

#include "Sim/Dumper/Dumper.h"

#include "Sim/Foundation/Hook/HookUtil.h"
#include "Sim/Memory/MemOrderManager/MemOrderManager.h"
#include "Sim/Memory/Cache/Cache.h"
#include "Sim/Memory/Cache/CacheSystem.h"
#include "Sim/Thread/Thread.h"
#include "Sim/InorderList/InorderList.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"

#include "Sim/Pipeline/Dispatcher/Dispatcher.h"
#include "Sim/Pipeline/Retirer/Retirer.h"
#include "Sim/Op/Op.h"

namespace Onikiri
{
    HookPoint<Core, Core::CheckpointDecisionHookParam> Core::s_checkpointDecisionHook;
};  // namespace Onikiri



using namespace std;
using namespace Onikiri;

Core::Core() :
    m_opArrayCapacity(0),
    m_timeWheelSize(0),
    m_globalClock(0),
    m_opArray(0),
    m_registerFile(0),
    m_fetcher(0),
    m_renamer(0),
    m_dispatcher(0),
    m_scheduler(0),
    m_retirer(0),
    m_latPred(0),
    m_cacheSystem(0),
    m_emulator(0),
    m_bPred(0),
    m_execLatencyInfo(0),
    m_loadPipeLineModel(LPM_MULTI_ISSUE),
    m_schedulerRemovePolicy(RP_FOLLOW_CORE),
    m_checkpointingPolicy(CP_AUTO)
{

}

Core::~Core()
{
    if( m_opArray != 0 )          delete m_opArray;
    ReleaseParam();
}

void Core::Initialize(InitPhase phase)
{
    if(phase == INIT_PRE_CONNECTION){
        LoadParam();

        m_opArray = new OpArray( m_opArrayCapacity );

        m_latencyPredRecv.Validate();
        m_addrMatchPredRecv.Validate();
        m_valuePredRecv.Validate();
        m_partialLoadRecovery.Validate();


    }
    else if(phase == INIT_POST_CONNECTION){

        CheckNodeInitialized( "registerFile", m_registerFile );
        CheckNodeInitialized( "fetcher", m_fetcher );
        CheckNodeInitialized( "renamer", m_renamer );
        CheckNodeInitialized( "dispatcher", m_dispatcher );
        CheckNodeInitialized( "retirer", m_retirer );
        CheckNodeInitialized( "latPred", m_latPred );
        CheckNodeInitialized( "execLatencyInfo", m_execLatencyInfo );
        CheckNodeInitialized( "thread", m_thread );
        CheckNodeInitialized( "cacheSystem", m_cacheSystem );
        CheckNodeInitialized( "emulator", m_emulator );

        for( int localTID = 0; localTID < GetThreadCount(); localTID++ ){
            m_thread[localTID]->SetLocalThreadID( localTID );
        }

        // scheduler の index が重複していないかチェック
        for(int i = 0; i < GetNumScheduler(); ++i) {
            for(int k = i + 1; k < GetNumScheduler(); ++k) {
                if( GetScheduler(i)->GetIndex() == GetScheduler(k)->GetIndex() ) {
                    THROW_RUNTIME_ERROR("same schduler index %d(%d, %d).",
                        GetScheduler(k)->GetIndex(), i, k);
                }
            }
        }

        // Check OpArray's capacity.
        int totalInorderListCapacity = 0;
        for( int i = 0; i < m_thread.GetSize(); ++i ){
            totalInorderListCapacity +=
                m_thread[i]->GetInorderList()->GetCapacity();
        }
        if( m_opArrayCapacity < totalInorderListCapacity ){
            THROW_RUNTIME_ERROR( 
                "The capacity(%d) of 'OpArray' is too small. Make 'Core/@OpArrayCapacity' more larger.",
                m_opArrayCapacity 
            );
        }
    }
}

// --- accessors
int Core::GetCID()
{
    return GetRID();
}

int Core::GetTID(const int index)
{
    return PhysicalResourceNode::GetTID( index );
}

void Core::AddScheduler( Scheduler* scheduler )
{
    m_scheduler.push_back( scheduler );
}

Scheduler* Core::GetScheduler( int index )
{
    ASSERT(
        index >= 0 && index < static_cast<int>(m_scheduler.size()),
        "illegal index: %d", index 
    );
    return m_scheduler[index];
}

int Core::GetNumScheduler() const
{
    return static_cast<int>(m_scheduler.size());
}

Thread* Core::GetThread(int tid)
{
    return m_thread[tid];     
}

int Core::GetThreadCount()
{
    return m_thread.GetSize();
}

//
// -- リカバリ方法
//
// この命令がフェッチされる前のインオーダーなステートをチェックポインティングする必要がある
// アクセスオーダーバイオレーションからの再フェッチによるリカバリに必要
bool Core::IsRequiredCheckpointBefore( const PC& pc, const OpInfo* const info)
{
    CheckpointDecisionHookParam param = { &pc, info, true/*before*/, false/*reqruired*/ } ;

    HOOK_SECTION_PARAM( s_checkpointDecisionHook, param ){
        if( m_checkpointingPolicy == CP_ALL ){
            param.requried = true;
        }
        else if ( m_checkpointingPolicy == CP_AUTO ){
            const OpClass& opClass = param.info->GetOpClass();
            param.requried = 
                m_latencyPredRecv.IsRequiredBeforeCheckpoint    ( opClass ) ||
                m_addrMatchPredRecv.IsRequiredBeforeCheckpoint  ( opClass ) ||
                m_valuePredRecv.IsRequiredBeforeCheckpoint      ( opClass ) ||
                m_partialLoadRecovery.IsRequiredBeforeCheckpoint( opClass );
        }
        else{
            ASSERT( "Unknown check-pointing policy." );
        }
    }

    return param.requried;
}

    // この命令がフェッチされた後のインオーダーなステートをチェックポインティングする必要がある
// 分岐予測ミスからのリカバリに必要
bool Core::IsRequiredCheckpointAfter( const PC& pc, const OpInfo* const info)
{
    CheckpointDecisionHookParam param = { &pc, info, false/*before*/, false/*reqruired*/ } ;
    HOOK_SECTION_PARAM( s_checkpointDecisionHook, param ){
        if( m_checkpointingPolicy == CP_ALL ){
            param.requried = true;
        }
        else if ( m_checkpointingPolicy == CP_AUTO ){
            const OpClass& opClass = param.info->GetOpClass();
            param.requried = 
                opClass.IsBranch() ||
                m_latencyPredRecv.IsRequiredAfterCheckpoint     ( opClass ) ||
                m_addrMatchPredRecv.IsRequiredAfterCheckpoint   ( opClass ) ||
                m_valuePredRecv.IsRequiredAfterCheckpoint       ( opClass ) ||
                m_partialLoadRecovery.IsRequiredAfterCheckpoint ( opClass );
        }
        else{
            ASSERT( "Unknown check-pointing policy." );
        }
    }

    return param.requried;
}

// Cycle handler
void Core::Evaluate()
{
    ClockedResourceBase::Evaluate();

    // Front-end stall decision
    Cache* lv1ICache = m_cacheSystem->GetFirstLevelInsnCache();
    if( lv1ICache->IsStallRequired() ){
        m_dispatcher->StallThisCycle();
    }
    // Back-end  stall decision
    Cache* lv1DCache = m_cacheSystem->GetFirstLevelDataCache();
    if( lv1DCache->IsStallRequired() ){

        m_retirer->StallThisCycle();
        for( size_t i = 0; i < m_scheduler.size(); i++ ){
            m_scheduler[i]->StallThisCycle();
        }
    }

}

