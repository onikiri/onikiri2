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

#include "Sim/ExecUnit/ExecUnitBase.h"

#include "Sim/Op/Op.h"
#include "Sim/Foundation/Hook/Hook.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/Pipeline/Scheduler/FinishEvent.h"
#include "Sim/Pipeline/Scheduler/DetectLatPredMissEvent.h"
#include "Sim/Pipeline/Pipeline.h"
#include "Sim/ExecUnit/ExecLatencyInfo.h"
#include "Sim/Dumper/Dumper.h"
#include "Sim/Pipeline/Scheduler/RescheduleEvent.h"
#include "Sim/Core/Core.h"
#include "Sim/Predictor/LatPred/LatPred.h"
#include "Sim/System/GlobalClock.h"

using namespace Onikiri;
using namespace std;

ExecUnitBase::ExecUnitBase()
{
    m_execLatencyInfo = 0;
    m_core = 0;
    m_numPorts = 0;
    m_numUsed = 0;
    m_numUsable = 0;
}

ExecUnitBase::~ExecUnitBase()
{
}

void ExecUnitBase::Initialize( InitPhase phase )
{
    if(phase == INIT_PRE_CONNECTION){
        LoadParam();
        for(size_t i = 0; i < m_codeStr.size(); i++){
            m_code.push_back( OpClassCode::FromString( m_codeStr[i] ) );
        }
    }
    else if(phase == INIT_POST_CONNECTION){
        CheckNodeInitialized( "execLatencyInfo", m_execLatencyInfo );
        CheckNodeInitialized( "core", m_core );

        m_reserver.Initialize( m_numPorts, m_core->GetTimeWheelSize() );
    }
}

void ExecUnitBase::Finalize()
{
    if( m_core ){
        m_numUsable = m_core->GetGlobalClock()->GetTick() * m_numPorts;
    }
    ReleaseParam();
}

int ExecUnitBase::GetMappedCode(int i)
{
    if((int)m_code.size() <= i)
        THROW_RUNTIME_ERROR("invalid mapped code.");
    return m_code[i];
}

int ExecUnitBase::GetMappedCodeCount()
{
    return (int)m_code.size();
}

void ExecUnitBase::Execute( OpIterator op )
{
    // ExecutionBegin() ではエミュレーションが行われる
    // 実行の正しさの検証を行うため--実行ステージの最初の段階で，
    // 正しいソース・オペランドが得られているかどうかを確かめるため--
    // execution stages の最初でエミュレーションを行う．

    // When a multi-issue mode is enabled, Execute() need to be called more than once,
    // because access order violation is detected from 'executed' load ops.
    // Re-scheduling changes the status of load ops to 'un-executed' and
    // cannot detect those load ops.
    op->ExecutionBegin();
    g_dumper.Dump( DS_EXECUTE, op );
    op->SetStatus( OpStatus::OS_EXECUTING );
}


void ExecUnitBase::RegisterEvents( OpIterator op, const int latency )
{
    IssueState issueState = op->GetIssueState();

    // Record a cycle time when execution is kicked initially. 
    // 'executionKicked' is cleared when op is canceled by the other ops.
    if( !issueState.executionKicked ){
        issueState.executionKickedTime =
            op->GetScheduler()->GetLowerPipeline()->GetNow();
        issueState.executionLatency = latency;
    }

    if( issueState.multiIssue ){
        // Multi issue mode
        const LatPredResult::Scheduling& schd = op->GetLatPredRsult().Get(0);
        if( schd.latency == latency ){
            RegisterFinishEvent( op, latency ); // 実行終了イベントの登録
        }
        else{
            RegisterRescheduleEvent( op, latency, &issueState );
        }
    }
    else{
        // Single issue mode
        if( issueState.executionKicked ){
            THROW_RUNTIME_ERROR( "An op is executed more than once." );
        }
        else{
            RegisterFinishEvent( op, latency ); // 実行終了イベントの登録
            RegisterDetectEvent( op, latency ); // 予測ミス検出イベントの登録
        }
    }

    issueState.executionKicked = true;
    op->SetIssueState( issueState );

}


void ExecUnitBase::RegisterFinishEvent( OpIterator op, const int latency )
{
    // finish イベントを登録する
    EventPtr finishEvent(
        OpFinishEvent::Construct( op ));
    // RegisterFinishEvent は命令の実行開始時(OpExecuteEvent)に呼ばれる
    // そのため Finish するのは実行レイテンシ-1 サイクル後
    op->AddEvent( 
        finishEvent,
        op->GetScheduler()->GetLowerPipeline(), 
        latency - 1 
    );
}

void ExecUnitBase::RegisterDetectEvent( OpIterator op, const int latency )
{
    const LatPredResult& predResult = op->GetLatPredRsult();

    // Detect events
    Scheduler* scheduler = op->GetScheduler();
    int wakeups = predResult.GetCount();
    for( int i = 0; i < wakeups - 1; i++ ){
        const LatPredResult::Scheduling& sched = predResult.Get( i );
        if( sched.wakeup ){

            EventPtr detectLatPredMiss(
                OpDetectLatPredMissEvent::Construct( op, 0, sched.latency, latency )
            );
            op->AddEvent(
                detectLatPredMiss,
                scheduler->GetLowerPipeline(),
                sched.latency - 1
            );
        }
    }
}

void ExecUnitBase::RegisterRescheduleEvent( OpIterator op, const int latency, IssueState* issueState )
{
    const LatPredResult::Scheduling& schd = op->GetLatPredRsult().Get(0);
    int minLatency = schd.latency;


    int elpasedTime = (int)( op->GetScheduler()->GetLowerPipeline()->GetNow() - issueState->executionKickedTime );
    int latencyOffset = op->GetScheduler()->GetIssueLatency() + 1 + minLatency;

    // Search next speculatively rescheduling timing.
    const LatPredResult& predResult = op->GetLatPredRsult();
    int wakeups = predResult.GetCount();
    int rsLatency = 0;
    bool speculative = false;
    for( int i = issueState->currentPredIndex + 1; i < wakeups; i++ ){
        const LatPredResult::Scheduling& sched = predResult.Get( i );
        if( sched.wakeup ){
            issueState->currentPredIndex = i;
            rsLatency = predResult.Get(i).latency - latencyOffset - elpasedTime;
            speculative = true;
            break;
        }
    }
            
    // There is no speculative rescheduling timing, wait for the latency.
    if( !speculative ){
        rsLatency = latency - latencyOffset;
    }

    if( rsLatency < 1 ){
        rsLatency = 1;
    }

    // When ops are speculatively waken-up, detection events need to be registered.
    if( (!issueState->executionKicked && schd.wakeup) || 
        issueState->executionKicked
    ){
        // Detection events need to occur before re-scheduling events occur,
        // because re-scheduling events cancel all the other events.
        if( rsLatency < minLatency ){
            rsLatency = minLatency;
        }

        // An op is issued when the op is finished for 'minLatency' except the first time,
        // so detection events are registered after 'minLatency'.
        EventPtr detectLatPredMiss(
            OpDetectLatPredMissEvent::Construct( op, 0, minLatency, latency )
        );
        op->AddEvent(
            detectLatPredMiss,
            op->GetScheduler()->GetLowerPipeline(),
            minLatency - 1
        );
    }

    // Re-scheduling events need to be registered after detection events.
    EventPtr evnt(
        OpRescheduleEvent::Construct(op)
    );
    op->AddEvent( evnt, op->GetScheduler()->GetLowerPipeline(), rsLatency, Op::EVENT_MASK_WAKEUP_RELATED );





}

// OpClass から取りうるレイテンシの種類の数を返す
int ExecUnitBase::GetLatencyCount(const OpClass& opClass)
{
    // 通常の演算器ではレイテンシは固定
    return 1;
}

// OpClass とインデクスからレイテンシを返す
int ExecUnitBase::GetLatency(const OpClass& opClass, int index)
{
    // 単にExecLatencyInfoをひいてかえす
    return m_execLatencyInfo->GetLatency(opClass.GetCode());
}

// 毎サイクル呼ばれる
void ExecUnitBase::Begin()
{
    m_reserver.Begin();
}

// Called in Update phase.
void ExecUnitBase::Update()
{
    m_reserver.Update();
}

