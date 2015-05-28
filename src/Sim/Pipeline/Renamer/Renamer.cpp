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

#include "Sim/Pipeline/Renamer/Renamer.h"

#include "Sim/Dumper/Dumper.h"

#include "Sim/Foundation/Hook/Hook.h"
#include "Sim/Foundation/Hook/HookUtil.h"
#include "Sim/Predictor/DepPred/RegDepPred/RegDepPredIF.h"
#include "Sim/Predictor/DepPred/MemDepPred/MemDepPredIF.h"
#include "Sim/Memory/MemOrderManager/MemOrderManager.h"
#include "Sim/Thread/Thread.h"
#include "Sim/Core/Core.h"

#include "Sim/Foundation/Checkpoint/CheckpointMaster.h"
#include "Sim/ISAInfo.h"
#include "Sim/Dependency/PhyReg/PhyReg.h"
#include "Sim/Register/RegisterFile.h"
#include "Sim/Op/Op.h"
#include "Sim/Pipeline/Dispatcher/Steerer/SteererIF.h"
#include "Sim/Predictor/LatPred/LatPred.h"

using namespace std;
using namespace boost;
using namespace Onikiri;

namespace Onikiri 
{
    HookPoint< Renamer > Renamer::s_renameEvaluateHook;
    HookPoint< Renamer > Renamer::s_renameUpdateHook;
    HookPoint< Renamer > Renamer::s_steerHook;
}


Renamer::Renamer() :
    m_renameLatency(0),
    m_steerer(0),
    m_latPred(0),
    m_numRenamedOps(0)
{
}

Renamer::~Renamer()
{
}

void Renamer::Initialize(InitPhase phase)
{
    PipelineNodeBase::Initialize(phase);

    if( phase == INIT_PRE_CONNECTION ){
        LoadParam();
    }
    else if( phase == INIT_POST_CONNECTION ){

        int threadCount = GetCore()->GetThreadCount();
        if( m_regDepPred.GetSize() != threadCount || 
            m_memDepPred.GetSize() != threadCount ||
            m_memOrderManager.GetSize() != threadCount
        ){
            THROW_RUNTIME_ERROR(
                "The counts of the "
                "'regDepPred' (%d), m_memDepPred(%d), memOrderManager(%d) "
                "are not equal to the thread coount of the core(%d)." ,
                m_regDepPred.GetSize(),
                m_memDepPred.GetSize(),
                m_memOrderManager.GetSize(),
                threadCount
            );
        }

        // member 変数のチェック
        if( m_renameLatency == 0 ){
            THROW_RUNTIME_ERROR( "rename latency is zero." );
        }
    }
}

void Renamer::Finalize()
{
    m_stallCycles.total = GetStalledCycles();

    m_stallCycles.others =
        m_stallCycles.total - 
        m_stallCycles.regDepPred -
        m_stallCycles.memDepPred -
        m_stallCycles.memOrderManager;

    ReleaseParam();
}

// array内のopを引数として，メンバ関数ポインタから呼び出しを行う
inline void Renamer::ForEachOp(
    RenamingOpArray* c, 
    void (Renamer::*func)(OpIterator) )
{
    for( RenamingOpArray::iterator i = c->begin(); i != c->end(); ++i ){
        (this->*func)(*i);
    }
}

template <typename T1>
inline void Renamer::ForEachOp1(
    RenamingOpArray* c,
    T1 arg1, 
    void (Renamer::*func)(OpIterator, T1) )
{
    for( RenamingOpArray::iterator i = c->begin(); i != c->end(); ++i ){
        (this->*func)( *i, arg1 );
    }
}

// リネームを行う関数
void Renamer::Rename( OpIterator op )
{
    // 依存関係の解決の前で呼ばれるHook
    HOOK_SECTION_OP( s_renameUpdateHook, op ){
        int tid = op->GetLocalTID();
        // 本来の処理
        m_regDepPred[ tid ]->Resolve( op );
        m_regDepPred[ tid ]->Allocate( op );

        m_memDepPred[ tid ]->Resolve( op );
        m_memDepPred[ tid ]->Allocate( op );
    }

    m_numRenamedOps++;
    m_opClassStat.Increment(op);
}

// Create an entry of MemOrderManager
void Renamer::CreateMemOrderManagerEntry( OpIterator op )
{
    if( op->GetOpClass().IsMem() ){
        op->GetThread()->GetMemOrderManager()->Allocate( op );
    }
}

// Predict latency of an op.
// Latency prediction is done on renaming stages.
void Renamer::Steer( OpIterator op )
{
    // Steering
    HOOK_SECTION_OP( s_steerHook, op )
    {
        Scheduler* sched = m_steerer->Steer( op );
        op->SetScheduler( sched );
    }

    // Process after renaming 
    // - Steer ops to schedulers.
    // - Predict latency of an op.
    //   Latency prediction is done on renaming stages.
    m_latPred->Predict( op );
}

void Renamer::BackupOnCheckpoint( OpIterator op, bool before )
{
    Core* core = op->GetCore();
    PC pc = op->GetPC();
    OpInfo* opInfo = op->GetOpInfo();
    
    if( before && core->IsRequiredCheckpointBefore( pc, opInfo ) ){
        CheckpointMaster* master = op->GetThread()->GetCheckpointMaster();
        master->Backup( op->GetBeforeCheckpoint(), CheckpointMaster::SLOT_RENAME );
    }

    if( !before && core->IsRequiredCheckpointAfter( pc, opInfo ) ){
        CheckpointMaster* master = op->GetThread()->GetCheckpointMaster();
        master->Backup( op->GetAfterCheckpoint(), CheckpointMaster::SLOT_RENAME );
    }
}

bool Renamer::CanRename( RenamingOpArray* renamingOps ) 
{
    if( renamingOps->size() == 0 ){
        THROW_RUNTIME_ERROR("Renaming 0 op.");
    }

    // Stall rename stages if there are any threads that can not allocate RMT etc.. entries.
    int numOps = 0;
    int numMemOps = 0;  // メモリ命令の数
    int localTID = renamingOps->at(0)->GetLocalTID();

    for( RenamingOpArray::iterator i = renamingOps->begin(); i != renamingOps->end(); ++i ){
        ASSERT( 
            (*i)->GetLocalTID() == localTID, 
            "Mixed thread ops can not be renamed in one cycle." 
        );
        numOps++;
        if( (*i)->GetOpClass().IsMem() ) {
            numMemOps++;
        }
    }
    
    // regDepPred のチェック
    if( !m_regDepPred[localTID]->CanAllocate( renamingOps->data(), numOps ) ) {
        ++m_stallCycles.regDepPred;
        return false;
    }

    // memDepPred のチェック
    if( !m_memDepPred[localTID]->CanAllocate( renamingOps->data(), numOps ) ) {
        ++m_stallCycles.memDepPred;
        return false;
    }

    // memOrderManager のチェック
    if( !m_memOrderManager[localTID]->CanAllocate( renamingOps->data(), numOps ) ) {
        ++m_stallCycles.memOrderManager;
        return false;
    }

    return true;
}

// Processing a NOP.
// NOP is not dispatched.
void Renamer::ProcessNOP( OpIterator op )
{
    ASSERT( op->GetDstRegNum() == 0, "NOP has a destination operand." );

    Core* core = op->GetCore();
    if( core->GetCheckpointingPolicy() == CP_ALL ){
        BackupOnCheckpoint( op, true );
        BackupOnCheckpoint( op, false );
    }

    // Set an op finished.
    op->SetStatus( OpStatus::OS_NOP );
    g_dumper.Dump( DS_COMMITTABLE, op );
}

// Enter an op to a renamer pipeline.
// An op is send to a dispatcher automatically when the op exits a renamer pipeline.
// A dispatcher is set to a renamer by the dispatcher itself in SetUpperPipelineNode
// that calls SetLowerPipelineNode of the renamer.
void Renamer::EnterPipeline( OpIterator op )
{
    GetLowerPipeline()->EnterPipeline( op, m_renameLatency - 1, GetLowerPipelineNode() );
    op->SetStatus( OpStatus::OS_RENAME );
    g_dumper.Dump( DS_RENAME, op );
}

// ClockedResource
void Renamer::Evaluate()
{
    BaseType::Evaluate();
    RenamingOpArray renamingOps;

    for( PipelineLatch::iterator i = m_latch.begin(); i != m_latch.end(); ++i ){
        HOOK_SECTION_OP( s_renameEvaluateHook, *i ){
            renamingOps.push_back( *i );
        }
    }
    
    if( renamingOps.size() > 0 && !CanRename( &renamingOps ) ){
        // Stall this node and upper pipelines.
        // Do not stall ops where renaming is already 
        // started before this cycle.
        StallThisNodeAndUpperThisCycle();
    } 
}

// Dispatch all ops in the latch if a renamer is not stalled.
void Renamer::Update()
{

    for( PipelineLatch::iterator i = m_latch.begin(); i != m_latch.end(); /*Do not ++i*/ ){

        RenamingOpArray ops;

        // Pack micro-ops that belong to one instruction to 'ops'
        // for processing one instruction as a unit.
        PC orgPC = (*i)->GetPC();
        do{
            ops.push_back( *i );
            i = m_latch.erase( i );
        }
        while( i != m_latch.end() && orgPC == (*i)->GetPC() );

        // Process NOP
        if( ops[0]->GetOpClass().IsNop() ) {
            ASSERT( ops.size() == 1, "NOP is divided to multiple ops." );
            ProcessNOP( ops[0] );
            continue;
        }

        // Create a checkpoint
        ForEachOp1( &ops, true/*before*/, &Renamer::BackupOnCheckpoint );

        // rename
        ForEachOp( &ops, &Renamer::Rename );

        // Create a checkpoint
        ForEachOp1( &ops, false/*before*/, &Renamer::BackupOnCheckpoint );

        // Create an entry of MemOrderManager
        ForEachOp( &ops, &Renamer::CreateMemOrderManagerEntry );

        // Do latency prediction
        ForEachOp( &ops, &Renamer::Steer );

        // All processes are done and now enter ops to pipeline.
        ForEachOp( &ops, &Renamer::EnterPipeline );

    }

}

void Renamer::Commit( OpIterator op )
{
    m_latPred->Commit( op );
}
