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

#include "Sim/Pipeline/Retirer/Retirer.h"

#include "Env/Env.h"
#include "Utility/RuntimeError.h"
#include "Sim/Dumper/Dumper.h"
#include "Sim/Op/OpContainer/OpList.h"
#include "Sim/Foundation/Hook/HookUtil.h"
#include "Sim/Op/Op.h"
#include "Sim/Core/Core.h"
#include "Sim/InorderList/InorderList.h"
#include "Sim/Thread/Thread.h"
#include "Sim/System/ForwardEmulator.h"
#include "Sim/Pipeline/Retirer/RetireEvent.h"
#include "Sim/Recoverer/Recoverer.h"

using namespace Onikiri;

// Define hook points.
namespace Onikiri 
{
    Retirer::CommitHookPoint            Retirer::s_commitHook;
    Retirer::RetireHookPoint            Retirer::s_retireHook;
    Retirer::CommitSteeringHookPoint    Retirer::s_commitSteeringHook;
    Retirer::CommitDecisionHookPoint    Retirer::s_commitDecisionHook;
};

void Retirer::Evaluated::Reset()
{
    committing.clear();
    exceptionOccur = false;
    exceptionCauser = OpIterator(0);
    evaluated = false;
    storePortFull = false;
}

Retirer::Retirer() :
    m_commitWidth(0),
    m_retireWidth(0),
    m_noCommitLimit(0),
    m_commitLatency(0),
    m_fixCommitLatency(false),
    m_committableStatus( OpStatus::OS_INVALID ),
    m_numCommittedOps(0),
    m_numCommittedInsns(0),
    m_numRetiredOps(0),
    m_numRetiredInsns(0),
    m_numStorePortFullStalledCycles(0),
    m_noCommittedCycle(0),
    m_numSimulationEndInsns(0),
    m_currentRetireThread(0),
    m_endOfProgram(false),
    m_emulator(0),
    m_forwardEmulator(0)
{
}

Retirer::~Retirer()
{
    ReleaseParam();
}

void Retirer::Initialize( InitPhase phase )
{
    PipelineNodeBase::Initialize( phase );

    if( phase == INIT_PRE_CONNECTION ){
        SetPriority( RP_COMMIT );
        LoadParam();
    }
    else if( phase == INIT_POST_CONNECTION ){
        DisableLatch();
    }

}

void Retirer::Evaluate()
{
    m_evaluated.Reset();
    EvaluateCommit();
    PipelineNodeBase::Evaluate();
}

// Evaluate commitment
void Retirer::EvaluateCommit()
{
    Thread* thread = GetCommitableThread();
    if( !thread ){
        m_evaluated.evaluated = true;
        return;
    }

    InorderList* inorderList = thread->GetInorderList();

    int comittedOps = 0;
    
    OpIterator headOp = inorderList->GetFrontOp();
    if( !headOp.IsNull() && headOp->GetException().exception ){
        m_evaluated.exceptionOccur = true;
        m_evaluated.exceptionCauser = headOp;

        // Sanity checking for commitment
        CheckCommitCounters( 0, inorderList );
        m_evaluated.evaluated = true;
        return;
    }

    // Check head ops in an in-order list and retire completed ops.
    for( OpIterator op = inorderList->GetFrontOp();
          !op.IsNull() && CanCommitInsn( op ) ;
         // Blank
    ){
        if( m_numSimulationEndInsns != 0 && 
            m_numCommittedInsns + comittedOps >= m_numSimulationEndInsns 
        ){
            break;
        }

        int microOpNum = op->GetOpInfo()->GetMicroOpNum();
        if( comittedOps + microOpNum > m_commitWidth ){
            break;
        }

        // Commit ops.
        for( int i = 0; i < microOpNum; i++ ){
            m_evaluated.committing.push_back( op );
            op = inorderList->GetNextIndexOp( op );
            comittedOps++;
        }
    }

    // Sanity checking for commitment
    CheckCommitCounters( comittedOps, inorderList );

    m_evaluated.evaluated = true;
    if( m_evaluated.storePortFull ){
        m_numStorePortFullStalledCycles++;
    }
}

void Retirer::Transition()
{
    BaseType::Transition();

    // On a stalled cycle, evaluated results must be cleared, because
    // the evaluated results may appear to be illegally flushed.
    if( IsStalledThisCycle() ){
        m_evaluated.Reset();
    }
}

// Update commitment
void Retirer::Update()
{
    if( m_numSimulationEndInsns != 0 && m_numRetiredInsns > m_numSimulationEndInsns ){
        THROW_RUNTIME_ERROR( "Simulation was done in excess of 'System/@SimulationInsns'" );
    }

    UpdateException();
    UpdateCommit();
}

// Set the number of retired ops/insns.
// This is called when a simulation mode is changed from an emulation mode.
void Retirer::SetInitialNumRetiredOp( s64 numInsns, s64 numOp, s64 simulationEndInsns )
{
    m_numCommittedInsns = numInsns;
    m_numCommittedOps   = numOp;

    m_numRetiredInsns   = numInsns;
    m_numRetiredOps     = numOp;

    m_numSimulationEndInsns = simulationEndInsns;
}

// Called from OpRetireEvent when the op is retired.
void Retirer::Retire( OpIterator op )
{
    HOOK_SECTION_OP( s_retireHook, op )
    {
        op->GetInorderList()->Retire( op );

        if( op->GetNo() == 0 ) {
            m_numRetiredInsns++;
        }
        m_numRetiredOps++;
    }
}

// Called when the op is flushed.
void Retirer::Flush( OpIterator op )
{
#if ONIKIRI_DEBUG
    CommitingOps* committing = &m_evaluated.committing;
    for( CommitingOps::iterator i = committing->begin(); i != committing->end(); ){
        // Committing ops must not be flushed.
        // This may occur when a wrong recovery policy is set.
        ASSERT( op != *i, "A committing op is flushed.\n%s", op->ToString().c_str() );
        ++i;
    }
    /*
    ASSERT(
        !(m_evaluated.exceptionOccur && m_evaluated.exceptionCauser == op ),
        "A exception causing op is flushed."
    );
    */
#endif
}

// Returns whether 'op' can retire or not.
bool Retirer::CanCommitOp( OpIterator op )
{
    CommitDecisionHookParam param = { op, false };
    HOOK_SECTION_OP_PARAM( s_commitDecisionHook, op, param )
    {
        param.canCommit = true;

        if( op->GetException().exception ){
            // Exception occurs.
            param.canCommit = false;
        }
        else{
            // An op can commit if the state of 'op' is 
            // after than FINISHED or COMPLETED.
            if( op->GetStatus() < m_committableStatus ){
                param.canCommit = false;
            }
            else{
                // Store
                const OpClass& opClass = op->GetOpClass();
                if( opClass.IsStore() ){
                    ExecUnitIF* execUnit = op->GetExecUnit();
                    if( execUnit->CanReserve( op, 0 ) ){
                        execUnit->Reserve( op, 0 );
                    }
                    else{
                        param.canCommit = false;
                        m_evaluated.storePortFull = true;
                    }
                }
            }
        }
    }

    return param.canCommit;
}

// Returns whether 'ops' that belongs to the same instruction can commit or not.
bool Retirer::CanCommitInsn( OpIterator op )
{
    OpInfo* opInfo = op->GetOpInfo();
    InorderList* inorderList = op->GetInorderList();

    int microOps = opInfo->GetMicroOpNum();
    OpIterator cur = inorderList->GetFrontOpOfSamePC( op );
    
    for( int i = 0; i < microOps; i++ ){
        ASSERT( cur->GetNo() == i, "Micro op indices mismatch." );
        if( cur.IsNull() || !CanCommitOp( cur ) ){
            return false;
        }
        cur = inorderList->GetNextIndexOp( cur );
    }

    return true;
}

// Decide a thread that is retired in this cycle.
Thread* Retirer::GetCommitableThread()
{
    CommitSteeringHookParam param = { &m_thread, NULL };
    HOOK_SECTION_PARAM( s_commitSteeringHook, param )
    {
        int index;
        int count = 0;
        int threadSize = m_thread.GetSize();
        do{
            index = m_currentRetireThread;
            m_currentRetireThread++;
            if( m_currentRetireThread >= threadSize ){
                m_currentRetireThread = 0;
            }

            count++;
            if( count > threadSize ){
                THROW_RUNTIME_ERROR( "No thread can retire." );
            }
        }
        while( !m_thread[index]->IsActive() );

        param.targetThread = m_thread[index];
    }
    return param.targetThread;
}

void Retirer::Commit( OpIterator op )
{
    HOOK_SECTION_OP( s_commitHook, op )
    {
        // Simulation is finished when a committed 'op' is a branch 
        // that jumps to an address 0.
        if( op->GetOpClass().IsBranch() && op->GetTakenPC().address == 0 ) {
            FinishThread( op->GetThread() );
        }

        m_forwardEmulator->OnCommit( op );
        m_emulator->Commit( &(*op), op->GetOpInfo() );
        op->GetInorderList()->Commit( op );

        // Decide the latency of commit and register a retire event. 
        int commitLatency = m_commitLatency;
        if( !m_fixCommitLatency && op->GetOpClass().IsStore() ){
            // The cache access of a store is done in MemOrderManager::Commit and 
            // its result is set to an op.
            int storeLatency = op->GetCacheAccessResult().latency;
            commitLatency = std::max( storeLatency, commitLatency );
        }
        EventPtr retireEvent( OpRetireEvent::Construct( op, this ) );
        op->AddEvent( retireEvent, GetLowerPipeline(), commitLatency );
    }
}

void Retirer::UpdateCommit()
{
    int committedOps = 0;
    for( CommitingOps::iterator i = m_evaluated.committing.begin(); i != m_evaluated.committing.end(); i++ ){

        OpIterator op = *i;

        // Commit an op.
        Commit( op );

        ++m_numCommittedOps;
        ++committedOps;
        m_opClassStat.Increment(op);

        // The number of committed instructions are incremented when a head 
        // op of an instruction is committed because multiple ops may be 
        // generated from a instruction.
        if( op->GetNo() == 0 ) {
            ++m_numCommittedInsns;
        }
    }

    // Count cycles from a cycle when a last op is retired.
    if( committedOps > 0 ) {
        m_noCommittedCycle = 0;
    } else {
        ++m_noCommittedCycle;
    }
}

void Retirer::UpdateException()
{
    if( m_evaluated.exceptionOccur ){
        OpIterator causer = m_evaluated.exceptionCauser;
        Recoverer* recoverer = causer->GetThread()->GetRecoverer();
        recoverer->RecoverException( causer );
        m_evaluated.exceptionOccur = false;
    }
}

// Check counters related to commit.
void Retirer::CheckCommitCounters( int committedOps, InorderList* inorderList )
{
    if( m_noCommittedCycle >= m_noCommitLimit ) {
        OpIterator uncommitted = inorderList->GetFrontOp();
        if( uncommitted.IsNull() ) {
            if( inorderList->IsEmpty() ){
                THROW_RUNTIME_ERROR(
                    "InOrderList is empty and no op commits while %d cycle(numRetiredOp = %d). ",
                    m_noCommittedCycle, m_numCommittedOps
                );
            }
            else{
                THROW_RUNTIME_ERROR(
                    "No op committed while %d cycle. A next retireable op( the front of InOrderList ) is %s",
                    m_noCommittedCycle, inorderList->GetCommittedFrontOp()->ToString(6).c_str() 
                );
            }
        }
        else {
            THROW_RUNTIME_ERROR(
                "No op committed while %d cycle. Next committable op( the front of InOrderList ) is %s",
                m_noCommittedCycle, uncommitted->ToString(6).c_str() 
            );
        }
    }
}

// Finish a thread.
void Retirer::FinishThread( Thread* tread )
{
    tread->Activate( false );
    Core* core = GetCore();

    // Search unfinished threads.
    bool active = false;
    for( int i = 0; i < core->GetThreadCount(); i++ ){
        if( core->GetThread(i)->IsActive() ){
            active = true;
        }
    }

    m_endOfProgram = !active;
}

