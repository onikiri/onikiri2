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

#include "Sim/Predictor/BPred/BPred.h"

#include "Utility/RuntimeError.h"
#include "Interface/OpClass.h"
#include "Sim/Dumper/Dumper.h"
#include "Sim/ISAInfo.h"
#include "Sim/Foundation/SimPC.h"
#include "Sim/Core/Core.h"
#include "Sim/Op/Op.h"

#include "Sim/Foundation/Hook/Hook.h"
#include "Sim/Foundation/Hook/HookUtil.h"

#include "Sim/Predictor/BPred/PHT.h"
#include "Sim/Predictor/BPred/DirPredIF.h"
#include "Sim/Predictor/BPred/GlobalHistory.h"
#include "Sim/Predictor/BPred/RAS.h"
#include "Sim/Recoverer/Recoverer.h"
#include "Sim/InorderList/InorderList.h"
#include "Sim/Thread/Thread.h"
#include "Sim/System/ForwardEmulator.h"

namespace Onikiri 
{
    HookPoint<BPred> BPred::s_branchPredictionMissHook;
};


using namespace Onikiri;
using namespace std;


BPred::Statistics::Statistics()
{
    numHit  = 0;
    numMiss = 0;
}

void BPred::Statistics::SetName( const String& name )
{
    this->name = name;
}


BPred::BPred()
{
    m_dirPred = 0;
    m_btb = 0;
    m_core = 0;
    m_fwdEmulator = 0;
    m_mode = SM_SIMULATION;
    m_perfect = false;

    BranchTypeUtility util;
    m_statistics.resize( util.GetTypeCount() );
    for( size_t i = 0; i < util.GetTypeCount(); i++){
        m_statistics[i].SetName( util.GetTypeName(i) );
    }
    m_totalStatistics.SetName( "All" );
}

BPred::~BPred()
{
    ReleaseParam();
}

// 初期化
void BPred::Initialize( InitPhase phase )
{
    if( phase == INIT_PRE_CONNECTION ){
        LoadParam();
    }
    else if( phase == INIT_POST_CONNECTION ){

        // メンバ変数が正しくセットされているかチェック
        CheckNodeInitialized( "dirPred", m_dirPred );
        CheckNodeInitialized( "btb",  m_btb );
        CheckNodeInitialized( "ras",  m_ras );
        CheckNodeInitialized( "core", m_core );
        CheckNodeInitialized( "forwardEmulator", m_fwdEmulator );

        if( !m_fwdEmulator->IsEnabled() ){
            THROW_RUNTIME_ERROR(
                "A perfect memory dependency predictor requires that a forawrd emulator is enabled." 
            );
        }

        m_btbPredTable.Resize( *m_core->GetOpArray() );
    }
}

// <TODO> 本当は，フェッチグループに対して次のフェッチグループを予測する

// op に対して、次に fetch される命令の PC を予測
PC BPred::Predict( OpIterator op, PC predIndexPC )
{
    if( m_perfect && m_mode == SM_SIMULATION ){
        // Forward emulator can work in a simulation mode only.
        const OpStateIF* result = m_fwdEmulator->GetExecutionResult( op );
        if( !result ){
            THROW_RUNTIME_ERROR( "Pre-executed result cannot be retrieved from a forward emulator." );
        }
        return
            result->GetTaken() ? result->GetTakenPC() : NextPC( result->GetPC() );
    }


    SimPC pc = op->GetPC();
    BTBPredict btbPred = m_btb->Predict(predIndexPC);
    bool btbHit     = btbPred.hit;

    m_btbPredTable[op] = btbPred;

    // BTBにヒットしなかった場合，分岐予測は（更新も含めて）行わない
    if(!btbHit)
        return pc.Next();


    PC branchTarget = btbPred.target;

    // 条件分岐の場合
    bool predTaken  = btbPred.dirPredict ? m_dirPred->Predict(op, predIndexPC) : true;

    switch(btbPred.type){
    case BT_NON:
        ASSERT(0, "BT_NON is invalid.");
        return pc.Next();

    case BT_CONDITIONAL:
        // taken / not taken の予測に応じて、次のPCを返す
        // not taken と予測した場合，次のopのPCを返す
        return predTaken ? branchTarget : pc.Next();        

    case BT_UNCONDITIONAL:
        // 無条件分岐なら BTB の予測を返す
        return branchTarget;

    case BT_CALL:
        // call なら RAS に push して、BTB の予測を返す
        // (インクリメントはPush内で行われる
        m_ras[op->GetLocalTID()]->Push(pc);
        return branchTarget;

    case BT_RETURN:
        // return なら RAS の予測を返す 
        return m_ras[op->GetLocalTID()]->Pop();

    case BT_CONDITIONAL_RETURN:
        // 条件付リターンの場合はDirPredをひいてTakenならPop        
        // not taken なら次のPCを返す
        return predTaken ? m_ras[op->GetLocalTID()]->Pop() : pc.Next();

    case BT_END:
        break;
    }

    // ここには未到達のはず
    THROW_RUNTIME_ERROR("reached end of Bpred::Predict\n");

    return pc.Next();   // warning よけ
}

// op の実行終了時に呼ばれる
void BPred::Finished( OpIterator op )
{
    if( m_perfect ){
        return;
    }

    const OpClass& opClass = op->GetOpClass();

    // 分岐でなければ何もしない
    if( !opClass.IsBranch() ) {
        return;
    }

    // Detect branch miss prediction and recovery if prediction is incorrect.
    RecoveryFromBPredMiss( op );

    // 条件分岐なら方向予測器に実行完了を通知
    // （予測時BTBヒットの場合のみ
    if( opClass.IsConditionalBranch() && m_btbPredTable[op].hit ){
        m_dirPred->Finished( op );
    }
}

// op のリタイア時に呼ばれる
void BPred::Commit( OpIterator op )
{
    if( m_perfect && m_mode != SM_SIMULATION ){
        return;
    }

    const OpClass& opClass = op->GetOpClass();
    if( !opClass.IsBranch() )
        return;

    bool conditional = opClass.IsConditionalBranch();
    
    if( !m_perfect ){
        // BTB更新
        const BTBPredict& predict = m_btbPredTable[op];
        m_btb->Update( op, predict );

        // 条件分岐なら方向予測器にリタイアを通知
        // （予測時BTBヒットの場合のみ
        if( conditional && predict.hit ){
            m_dirPred->Retired( op );
        }
    }

    // ヒット率
    PC pcTaken  = op->GetTakenPC();
    PC pcPred   = op->GetPredPC();
    PC pcNext   = NextPC( op->GetPC() );
    bool taken  = conditional ? op->GetTaken() : true;
    PC pcResult = taken ? pcTaken : pcNext;

    BranchTypeUtility util;
    BranchType type = util.OpClassToBranchType( opClass );
    if( pcPred == pcResult ){
        m_statistics[type].numHit++;
        m_totalStatistics.numHit++;
    }
    else{
        m_statistics[type].numMiss++;
        m_totalStatistics.numMiss++;
    }

}

// Detect branch miss prediction and recovery if prediction is incorrect.
void BPred::RecoveryFromBPredMiss( OpIterator branch )
{
    // Recovery is not necessary when the simulator is in an in-order mode.
    if( m_mode != PhysicalResourceNode::SM_SIMULATION ){
        return;
    }

    // If a perfect mode is enabled, prediction results are always correct and 
    // there is not nothing to do.
    if( IsPerfect() ){
        return;
    }

    if( branch->GetPredPC() != branch->GetNextPC() ) {
        // A branch prediction result is incorrect and recovery from an incorrect path.
        g_dumper.Dump( DS_BRANCH_PREDICTION_MISS, branch );
        HOOK_SECTION_OP( s_branchPredictionMissHook, branch )
        {
            Recoverer* recoverer = branch->GetThread()->GetRecoverer();
            recoverer->RecoverBPredMiss( branch );
        }
    }
}
