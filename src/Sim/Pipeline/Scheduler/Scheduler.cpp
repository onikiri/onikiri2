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


// Scheduler
// This class schedules ops on processor back-end stages.
// (from wakeup/select to write back).

#include <pch.h>

#include "Sim/Pipeline/Scheduler/Scheduler.h"

#include "Env/Env.h"
#include "Sim/Dumper/Dumper.h"

#include "Sim/Pipeline/Scheduler/IssueEvent.h"
#include "Sim/Pipeline/Scheduler/WakeUpEvent.h"
#include "Sim/Pipeline/Scheduler/ExecuteEvent.h"
#include "Sim/Pipeline/Scheduler/RescheduleEvent.h"
#include "Sim/Pipeline/Scheduler/WriteBackEvent.h"
#include "Sim/Pipeline/Scheduler/IssueState.h"
#include "Sim/Pipeline/Scheduler/DumpCommittableEvent.h"
#include "Sim/Pipeline/Scheduler/DumpSchedulingEvent.h"
#include "Sim/Pipeline/Scheduler/IssueSelector/IssueSelectorIF.h"

#include "Sim/Core/Core.h"
#include "Sim/Predictor/LatPred/LatPredResult.h"
#include "Sim/ExecUnit/ExecUnitIF.h"

#include "Sim/ExecUnit/ExecLatencyInfo.h"
#include "Sim/Register/RegisterFile.h"
#include "Sim/Op/Op.h"
#include "Sim/Core/DataPredTypes.h"

#include "Sim/Foundation/Hook/HookUtil.h"

using namespace std;
using namespace boost;
using namespace Onikiri;

namespace Onikiri 
{
    HookPoint<Scheduler> Scheduler::s_dispatchedHook;
    HookPoint<Scheduler> Scheduler::s_readySigHook;
    HookPoint<Scheduler> Scheduler::s_wakeUpHook;
    HookPoint<Scheduler> Scheduler::s_selectHook;
    HookPoint<Scheduler> Scheduler::s_issueHook;
    HookPoint<Scheduler, Scheduler::RescheduleHookParam> Scheduler::s_rescheduleHook;
};  // namespace Onikiri


Scheduler::Scheduler() :
    m_index(0),
    m_issueWidth(0),
    m_issueLatency(0),
    m_communicationLatency(0),
    m_windowCapacity(0),
    m_loadPipelineModel( LPM_INVALID ),
    m_removePolicyParam( RP_FOLLOW_CORE ),
    m_removePolicy( RP_FOLLOW_CORE ),
    m_selector( NULL )
{
}

Scheduler::~Scheduler()
{
    ReleaseParam();
}

void Scheduler::Initialize(InitPhase phase)
{
    // 基底クラスの初期化
    BaseType::Initialize(phase);

    if(phase == INIT_PRE_CONNECTION){
        LoadParam();
    }
    else if(phase == INIT_POST_CONNECTION){

        CheckNodeInitialized( "selector", m_selector );

        // OpList の初期化
        OpArray* opArray = GetCore()->GetOpArray();
        m_notReadyOp.resize( *opArray );
        m_readyOp.resize( *opArray );
        m_issuedOp.resize( *opArray );

        AddChild( &m_issuedOp );

        // Dumping stall is done on buffers.
        GetLowerPipeline()->EnableDumpStall( false );
        //m_latch.EnableDumpStall( false );

        // Initialize code map
        for( size_t i = 0; i < m_execUnit.size(); i++){
            int codeCount = m_execUnit[i]->GetMappedCodeCount();

            for(int j = 0; j < codeCount; j++){
                int code = m_execUnit[i]->GetMappedCode( j );
            
                // code が末尾のインデックスになるように拡張
                if((int)m_execUnitCodeMap.size() <= code)
                    m_execUnitCodeMap.resize(code + 1);

                ASSERT( m_execUnitCodeMap[code] == 0, "execUnit set twice(code:%d).", code);
                m_execUnitCodeMap[code] = m_execUnit[i];
            }
        }

        // Initialize scheduler cluster
        int numScheduler = GetCore()->GetNumScheduler();
        m_clusters.resize( numScheduler );
        for( int i = 0; i < numScheduler; i++ ){
            Cluster* cluster = &m_clusters[i];
            cluster->communicationLatency = m_communicationLatency[i];
            cluster->scheduler      = GetCore()->GetScheduler(i);
            cluster->issueLatency   = cluster->scheduler->GetIssueLatency();
        }

        m_loadPipelineModel = GetCore()->GetLoadPipelineModel();
        
        if( m_removePolicyParam == RP_FOLLOW_CORE ){
            m_removePolicy = GetCore()->GetSchedulerRemovePolicy();
        }
        else{
            m_removePolicy = m_removePolicyParam;
        }

        // Push all data prediction policies.
        typedef vector< const DataPredMissRecovery * > Preds;
        Preds preds;
        preds.push_back( &GetCore()->GetAddrPredMissRecovery() );
        preds.push_back( &GetCore()->GetLatPredMissRecovery() );
        preds.push_back( &GetCore()->GetValuePredMissRecovery() );
        preds.push_back( &GetCore()->GetPartialLoadRecovery() );

        // Check data prediction policies.
        if( m_removePolicy == RP_REMOVE ) {
            for( Preds::iterator i = preds.begin(); i != preds.end(); ++i ){
                if( !(*i)->IsRefetch() ){
                    THROW_RUNTIME_ERROR( "'Remove' scheduler only supports 'Refetch'." );
                }
            }
        }
        if( m_removePolicy == RP_REMOVE_AFTER_FINISH ) {
            for( Preds::iterator i = preds.begin(); i != preds.end(); ++i ){
                if( !( (*i)->IsRefetch() || (*i)->IsReissueNotFinished() || (*i)->IsReissueSelective() ) ){
                    THROW_RUNTIME_ERROR(
                        "'RemoveAfterFinish' scheduler only supports"
                        "'Refetch' or 'ReissueNotFinished' or 'ReissueSelective'." 
                    );
                }
            }
        }

        // communication latency の数のチェック
        if( static_cast<int>(m_communicationLatency.size()) != GetCore()->GetNumScheduler() ) {
            THROW_RUNTIME_ERROR("communication latency count != scheduler count");
        }
    }

}

void Scheduler::SetExecUnit( PhysicalResourceArray<ExecUnitIF>& execUnit )
{
    // 見つからなかったら登録
    if( find(m_execUnit.begin(), m_execUnit.end(), execUnit[0]) != m_execUnit.end() ){
        THROW_RUNTIME_ERROR("Same exec unit is set twice");
    }
    m_execUnit.push_back( execUnit[0] );
}

// code に対応するExecUnitを返す
ExecUnitIF* Scheduler::GetExecUnit(int code)
{
    ASSERT(code >= 0 && code < static_cast<int>(m_execUnitCodeMap.size()),
        "unknown code %d.", code);

    ASSERT(m_execUnitCodeMap[code] != 0, "no unit (code = %d).", code);

    return m_execUnitCodeMap[code];
}


void Scheduler::Begin()
{
    ASSERT( m_latch.size() == 0, "The latch is received ops from dispatcher." );

    // WakeupEvent::Evaluate() is triggered before Scheduler::Evaluate(),
    // thus clearing evaluated context is done before an evaluation phase.
    ClearEvaluated();

    // Call Begin() handlers of execution units.
    typedef std::vector<ExecUnitIF*>::iterator iterator;
    for( iterator i = m_execUnit.begin(); i != m_execUnit.end(); ++i ){
        (*i)->Begin();
    }

    BaseType::Begin();
}

void Scheduler::Evaluate()
{
    EvaluateWakeUp();   // Wake up
    EvaluateSelect();   // Select
    BaseType::Evaluate();
}

void Scheduler::Transition()
{
    BaseType::Transition();

    // On a stalled cycle, evaluated results must be cleared, because
    // the evaluated results may incorrectly set dispatched ops ready.
    if( IsStalledThisCycle() ){
        ClearEvaluated();
    }
}
void Scheduler::Update()
{
    UpdateWakeUp();     // Wake up
    UpdateSelect();     // Select

    // Call Update() handlers of execution units.
    typedef std::vector<ExecUnitIF*>::iterator iterator;
    for( iterator i = m_execUnit.begin(); i != m_execUnit.end(); ++i ){
        (*i)->Update();
    }
}


void Scheduler::Commit( OpIterator op )
{
    if( !op->IsDispatched() ){
        return;
    }

#ifdef ONIKIRI_DEBUG
    ASSERT(
        op->GetScheduler() == this,
        "The op does not belong to this scheduler" 
    );

    if( m_loadPipelineModel == LPM_SINGLE_ISSUE ){
        ASSERT(
            m_issuedOp.count(op) > 0,
            "Attempted to commit a not issued op. %s", op->ToString(5).c_str()
        );
    }
    else{
        ASSERT(
            op->GetStatus() > OpStatus::OS_ISSUING,
            "Attempted to commit a not issued op. %s", op->ToString(5).c_str()
        );
    }
#endif
}

void Scheduler::Flush( OpIterator op )
{
    BaseType::Flush( op );
    Delete( op );
}

void Scheduler::Retire( OpIterator op )
{
    BaseType::Retire( op );
    Delete( op );
}

void Scheduler::Delete( OpIterator op )
{
    OpStatus status = op->GetStatus();
    if( status < OpStatus::OS_DISPATCHING ){
        // The status of ops in m_dispatchedOp is OS_DISPATCHING.
        return;
    }

    ASSERT(
        op->GetScheduler() == this,
        "The op does not belong to this scheduler" 
    );

    if( m_issuedOp.find_and_erase(op) ){
        return;
    }
    else if( m_readyOp.find_and_erase(op) ) {
        return;
    }
    else if( m_notReadyOp.find_and_erase(op) ) {
        return;
    }

    //THROW_RUNTIME_ERROR("The op not found in this scheduler");
}

// Cancel
void Scheduler::Cancel( OpIterator op )
{
    int dstDepNum = op->GetDstDepNum();
    for( int i = 0; i < dstDepNum; ++i ){
        Dependency* dep = op->GetDstDep( i );
        for( DependencySet::iterator d = m_evaluated.deps.begin(); d != m_evaluated.deps.end();  ){
            if( *d == dep ){
                d = m_evaluated.deps.erase( d );
            }
            else{
                ++d;
            }
        }
    }
    for( SchedulingOps::iterator i = m_evaluated.selected.begin(); i != m_evaluated.selected.end();  ){
        if( *i == op ){
            i = m_evaluated.selected.erase( i );
        }
        else{
            ++i;
        }
    }
    for( SchedulingOps::iterator i = m_evaluated.wokeUp.begin(); i != m_evaluated.wokeUp.end();  ){
        if( *i == op ){
            i = m_evaluated.wokeUp.erase( i );
        }
        else{
            ++i;
        }
    }
}

// dispatchされてきた op をうけとる
void Scheduler::ExitUpperPipeline( OpIterator op )
{
    HookEntry(
        this,
        &Scheduler::DispatchEnd,
        &s_dispatchedHook,
        op
    );

    // Do not call the PipelineNodeBase::ExitUpperPipeline becasue
    // the op is pushed to a latch in the ExitUpperPipeline.
    //
    // Dispatchers dispatch ops to schedulers not depending on the 
    // stall of the schedulers, but the free space of those.
    // When a scheduler is stalled, its latch cannot receive ops 
    // and assertion fails. Thus a scheduler receives ops to m_dispatchedOp.
}

void Scheduler::EvaluateDependency( OpIterator producer )
{
    ASSERT( GetCurrentPhase() == PHASE_EVALUATE );

    int dstDepNum = producer->GetDstDepNum();

    for( int i = 0; i < dstDepNum; ++i ){
        Dependency* dep = producer->GetDstDep( i );
        const Dependency::ConsumerListType& consumers = dep->GetConsumers();
        typedef Dependency::ConsumerListType::const_iterator ConsumerListIterator;
        for( ConsumerListIterator c = consumers.begin(); c != consumers.end(); ++c ){
            g_dumper.DumpOpDependency( producer, *c );
        }
        m_evaluated.deps.push_back( dep );
    }
}

// Register wakeup events for waking up consumers of 'op'
// Note: 'latencyFromOp' does not specifies latency from now but
// latency between a producer and consumers.
void Scheduler::RegisterWakeUpEvent( OpIterator op, int latencyFromOp )
{
    int numScheduler = (int)m_clusters.size();
    for(int i = 0; i < numScheduler; ++i) {
        Cluster* cluster = &m_clusters[i];
        
        // communication latency が -1 なら wakeup しない
        int comLatency = cluster->communicationLatency;
        if (comLatency == -1) 
            continue;

        Scheduler* targetScheduler = cluster->scheduler;
        Pipeline* targetPipeline   = targetScheduler->GetLowerPipeline();
        int targetIssueLatency     = cluster->issueLatency;

        // back to back に実行するため、Scheduler 間の issue latency の差も考慮する
        // ただし、wakeup には最低でも communication latency or 1サイクルの時間はかかる
        int wakeupLatency = latencyFromOp + m_issueLatency - targetIssueLatency;
        if( wakeupLatency < comLatency ){
            wakeupLatency = comLatency;
        }
        // If m_issueLatency < targetIssueLatency, wakeupLatency may be 0 cycle.
        // Consumers cannot be woke-up at the same cycle, so consumers in the
        // target scheduler are woke up at the next cycle.
        if (wakeupLatency < 1) {
            wakeupLatency = 1;
        }

        op->AddEvent(
            OpWakeUpEvent::Construct(
                targetScheduler,
                op
            ), 
            targetPipeline,
            wakeupLatency,
            Op::EVENT_MASK_WAKEUP_RELATED
        );
    }
}


//
// Issue
//
void Scheduler::IssueBody(OpIterator op)
{
    IssueState state = op->GetIssueState();
    if( op->GetStatus() >= OpStatus::OS_ISSUING && state.multiIssue ){
    }
    else{
        // Set a status to ISSUING.
        op->SetStatus( OpStatus::OS_ISSUING );
    }

#ifdef ONIKIRI_DEBUG
    if(op->GetScheduler() != this) {
        THROW_RUNTIME_ERROR("issued an unknown op.");
    }
    if( !m_issuedOp.count(op) ) {
        if( m_notReadyOp.count(op) ) {
            THROW_RUNTIME_ERROR("issued not ready op\t%s\n", op->ToString().c_str());
        }else if( m_readyOp.count(op) ) {
            THROW_RUNTIME_ERROR("issued already not selected op\t%s\n", op->ToString().c_str());
        }else {
            THROW_RUNTIME_ERROR("issued ? op.");
        }
    }
#endif

    g_dumper.Dump(DS_ISSUE, op);
    m_issuedOpClassStat.Increment(op);
}

void Scheduler::Issue(OpIterator op)
{
    HookEntry(
        this,
        &Scheduler::IssueBody,
        &s_issueHook,
        op
    );
}

//
// Finish
//
void Scheduler::Finished( OpIterator op )
{
    // Cancel all wake up related events and wake up consumers immediately.
    op->CancelEvent( Op::EVENT_MASK_WAKEUP_RELATED );

#if 0
    RegisterWakeUpEvent( op, 1 );
#else

    bool needWakeup = false;

    int dstDepNum = op->GetDstDepNum();
    for( int i = 0; i < dstDepNum; ++i ){
        if( !op->GetDstDep(i)->IsFullyReady() ){
            needWakeup = true;
            break;
        }
    }

    if( needWakeup ){
        RegisterWakeUpEvent( op, 1 );
    }

#endif
    
    // Register a register write back event.
    op->AddEvent( 
        OpWriteBackEvent::Construct( op, this, true /*wbBegin*/ ),
        GetLowerPipeline(), 
        1   // latency
    );
}

// Write Back
void Scheduler::WriteBackBegin( OpIterator op )
{
    op->SetStatus( OpStatus::OS_WRITING_BACK );
    g_dumper.Dump( DS_WRITEBACK, op );

    op->AddEvent( 
        OpWriteBackEvent::Construct( op, this, false    /*wbBegin*/ ),
        GetLowerPipeline(), 
        m_writeBackLatency - 1      // latency
                                    // -1 corresponds this cycle.
    );
}

void Scheduler::WriteBackEnd( OpIterator op )
{
    op->SetStatus( OpStatus::OS_WRITTEN_BACK );
    if( g_dumper.IsEnabled() ){
        op->AddEvent( OpDumpCommittableEvent::Construct( op ), GetLowerPipeline(), 1 );
    }
}

//
// Reschedule
//
bool Scheduler::Reschedule( OpIterator op )
{
    if( !op->IsDispatched() ){
        return false;
    }


    RescheduleHookParam param;
    param.canceled = false;

    ASSERT(
        op->GetScheduler() == this,
        "rescheduled an unknown op."
    );

    HOOK_SECTION_OP_PARAM( s_rescheduleHook, op, param )
    {
        // 再スケジューリングのポリシーごとにここは書き換えられるようにする

        bool clearIssueState = m_loadPipelineModel == LPM_SINGLE_ISSUE;
        op->RescheduleSelf( clearIssueState );

        // Cancel 
        Cancel( op );

        param.canceled = false;
        if( m_issuedOp.find_and_erase(op) ) {
            // 発行済みの命令が再スケジューリングされた

            if(op->IsSrcReady(GetIndex())) {
                m_readyOp.push_inorder(op);
            }
            else {
                m_notReadyOp.push_back(op);
            }
            param.canceled = true;
        }
        else if (m_readyOp.count(op)) {
            // 発行前だがreadyになっていた命令が再スケジューリングされた
            if( !op->IsSrcReady(GetIndex()) ) {
                m_readyOp.erase(op);
                m_notReadyOp.push_back(op);
            }
        }
        else {
            // not ready だった命令は再スケジューリングされてもなにもしない 
        }

        op->SetStatus(OpStatus::OS_DISPATCHED);
        g_dumper.Dump(DS_RESCHEDULE, op);
    }
    return param.canceled;
}


// Return whether a scheduler can dispatch 'ops' or not.
bool Scheduler::CanAllocate( int ops )
{
    return ( GetOpCount() + ops ) <= m_windowCapacity;
}

// Clear evaluated conctext.
void Scheduler::ClearEvaluated()
{
    m_evaluated.deps.clear();
    m_evaluated.selected.clear();
    m_evaluated.wokeUp.clear();
}

// dispatch されてきた op をうけとる
void Scheduler::DispatchEnd( OpIterator op )
{
    if( GetOpCount() >= m_windowCapacity ) {
        THROW_RUNTIME_ERROR("scheduler cannot receive");
    }

    // すでに ready になっているかの判定
    if( op->IsSrcReady( GetIndex(), &m_evaluated.deps ) ) {
        m_readyOp.push_inorder(op);
    }
    else {
        m_notReadyOp.push_back(op);
    }

    op->SetStatus( OpStatus::OS_DISPATCHED );
    if( g_dumper.IsEnabled() ){
        op->AddEvent( OpDumpSchedulingEvent::Construct( op ), GetLowerPipeline(), 1 );
    }
}


//
// --- wake up
// 

void Scheduler::CheckOnWakeUp(OpIterator op)
{
#ifdef ONIKIRI_DEBUG
    if(!op->IsDispatched()){
        THROW_RUNTIME_ERROR("woke up an op that is not dipatched.");
    }
    if(op->GetScheduler() != this) {
        THROW_RUNTIME_ERROR("woke up an unknown op.");
    }
    
    if (m_readyOp.count(op)) {
        THROW_RUNTIME_ERROR("woke up a ready op\t%s\n", op->ToString().c_str());
    }
    else if (m_issuedOp.count(op)) {
        THROW_RUNTIME_ERROR("woke up an issued op\t%s\n", op->ToString().c_str());
    }
    else if (!m_notReadyOp.count(op)) {
        THROW_RUNTIME_ERROR("woke up an unknown op.");
    }

#endif

}

void Scheduler::EvaluateWakeUp()
{
    DependencySet depSet;
    int index = GetIndex();
    for( DependencySet::iterator d = m_evaluated.deps.begin(); d != m_evaluated.deps.end(); ++d ){
        depSet.push_back( *d );

        const Dependency::ConsumerListType& consumers = (*d)->GetConsumers();
        typedef Dependency::ConsumerListType::const_iterator ConsumerListIterator;

        ConsumerListIterator end = consumers.end();
        for( ConsumerListIterator c = consumers.begin(); c != end; ++c ){
            OpIterator op = *c;
            if( op->IsDispatched() && 
                op->GetScheduler()->GetIndex() == index &&
                op->IsSrcReady( index, &depSet ) 
            ){
                // A woke up op is determined.
                m_evaluated.wokeUp.push_back( op );
            }
        }
    }
}

void Scheduler::UpdateWakeUp()
{
    // Set a ready signal.
    for( DependencySet::iterator i = m_evaluated.deps.begin(); i != m_evaluated.deps.end(); ++i ){
        // Destination of a producer is set to ready.
        (*i)->SetReadiness( true, GetIndex() );
    }

    // Wakeup consumers.
    for( SchedulingOps::iterator i = m_evaluated.wokeUp.begin(); i != m_evaluated.wokeUp.end(); ++i ){
        WakeUp( *i );
    }
}

void Scheduler::WakeUp(OpIterator op)
{
    CheckOnWakeUp( op );

    HOOK_SECTION_OP( s_readySigHook, op ){
        g_dumper.Dump(DS_READY_SIG, op);
    }

    if( op->IsSrcReady(GetIndex()) ){
        HOOK_SECTION_OP( s_wakeUpHook, op ){
            g_dumper.Dump(DS_WAKEUP, op);
            m_notReadyOp.erase(op);
            m_readyOp.push_inorder(op);
        }
    }
}


//
// --- Select
//
void Scheduler::EvaluateSelect()
{
    m_selector->EvaluateSelect( this );
}

void Scheduler::UpdateSelect()
{
    // Select and issue ops.
    for( SchedulingOps::iterator i = m_evaluated.selected.begin(); i != m_evaluated.selected.end(); ++i ){
        m_readyOp.erase( *i );
        m_issuedOp.push_back( *i );
        Select( *i );
    }
}

void Scheduler::SelectBody(OpIterator op)
{
    // Reserve an execution unit.
    const LatPredResult& predResult = op->GetLatPredRsult();
    IssueState issueState = op->GetIssueState();

    if( m_loadPipelineModel == LPM_MULTI_ISSUE ){
        // Multi issue mode:
        // In a multi issue mode, ops are re-scheduled and issued 
        // when their consumers need to be waken-up, 
        // so always wakeup its consumer when an op is selected.
        issueState.multiIssue = true;

        const LatPredResult::Scheduling& sched = predResult.Get( 0 );
        if( (!issueState.issued && sched.wakeup) || issueState.issued ){
            // In case of second or later issue,
            // wakeup is always done at the earliest timing.
            RegisterWakeUpEvent( op, sched.latency );
        }
    }
    else{
        // Single issue mode:
        // In a single issue mode, register wakeup-events at all possible timings.
        issueState.multiIssue = false;

        int wakeups = predResult.GetCount();
        for( int i = 0; i < wakeups; i++ ){
            const LatPredResult::Scheduling& sched = predResult.Get( i );
            if( sched.wakeup ){
                RegisterWakeUpEvent( op, sched.latency );
            }
        }
    }
    
    // Register an issue event at the next cycle.
    EventPtr issueEvent( OpIssueEvent::Construct( op ) );
    op->AddEvent(issueEvent, GetLowerPipeline(), 1 );

    // Register an execution event after an issue latency + 1(for select<>issue time).
    RegisterExecuteEvent( op, m_issueLatency + 1 );

    // Update issue state.
    issueState.issued = true;
    op->SetIssueState( issueState );

    g_dumper.Dump( DS_SELECT, op );

}

void Scheduler::Select(OpIterator op)
{
    HookEntry(
        this,
        &Scheduler::SelectBody,
        &s_selectHook,
        op
    );
}

// Returns whether 'op' can be selected in this cycle.
// This method must be called only in a 'Evaluate' phase.
bool Scheduler::CanSelect( OpIterator op )
{
    ASSERT( GetCurrentPhase() == PHASE_EVALUATE );
    // Check whether execution units can be reserved or not after the latency of issue.
    // +1 is for the first execution stage.
    return op->GetExecUnit()->CanReserve( op, m_issueLatency + 1 );
}

// Reserves 'op' to select in this cycle.
// This method must be called only in a 'Evaluate' phase.
void Scheduler::ReserveSelect( OpIterator op )
{
    ASSERT( GetCurrentPhase() == PHASE_EVALUATE );
    op->GetExecUnit()->Reserve( op, m_issueLatency + 1 );   // +1 is for the first execution stage.
    m_evaluated.selected.push_back( op );
}


int Scheduler::GetOpCount()
{
    int size = static_cast<int>( m_notReadyOp.size() + m_readyOp.size() );
    if( m_removePolicy == RP_RETAIN ) {
        size += static_cast<int>( m_issuedOp.size() );
    }
    else if( m_removePolicy == RP_REMOVE_AFTER_FINISH ){
        for( OpBuffer::iterator i = m_issuedOp.begin(); i != m_issuedOp.end(); ++i ){
            OpIterator op = *i;
            if( op->GetStatus() <= OpStatus::OS_EXECUTING ){
                size++;
            }
        }
    }
    return size;
}

bool Scheduler::IsInScheduler( OpIterator op )
{
    bool isInSched = (m_notReadyOp.count(op) || m_readyOp.count(op));
    if( m_removePolicy == RP_RETAIN ) {
        isInSched = (isInSched || m_issuedOp.count(op));
    }
    else if( m_removePolicy == RP_REMOVE_AFTER_FINISH ){
        if( op->GetStatus() <= OpStatus::OS_EXECUTING ){
            isInSched = (isInSched || m_issuedOp.count(op));
        }
    }
    return isInSched;
}

// Register execute events for 'op'.
// Note: 'latencyFromNow' specifies latency from now.
void Scheduler::RegisterExecuteEvent( OpIterator op, int latencyFromNow )
{
    op->AddEvent(
        OpExecuteEvent::Construct( op ),
        GetLowerPipeline(), 
        latencyFromNow
    );
}
