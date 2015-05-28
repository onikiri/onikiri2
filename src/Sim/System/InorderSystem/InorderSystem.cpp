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
#include "Sim/System/InorderSystem/InorderSystem.h"
#include "Sim/Dumper/Dumper.h"

#include "Sim/Predictor/DepPred/RegDepPred/RegDepPredIF.h"
#include "Sim/Predictor/BPred/BPred.h"
#include "Sim/Predictor/LatPred/LatPredResult.h"
#include "Sim/InorderList/InorderList.h"
#include "Sim/Register/RegisterFile.h"
#include "Sim/Memory/Cache/Cache.h"
#include "Sim/Memory/Cache/CacheSystem.h"
#include "Sim/Op/Op.h"
#include "Sim/Op/OpInitArgs.h"
#include "Sim/Pipeline/Fetcher/Fetcher.h"
#include "Sim/Pipeline/Dispatcher/Steerer/OpCodeSteerer.h"
#include "Sim/Pipeline/Renamer/Renamer.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/Predictor/HitMissPred/HitMissPredIF.h"
#include "Sim/Predictor/LatPred/LatPred.h"
#include "Sim/Dependency/PhyReg/PhyReg.h"
#include "Sim/Memory/MemOrderManager/MemOrderManager.h"

using namespace std;
using namespace boost;
using namespace Onikiri;

//
// --- Run
//

InorderSystem::InorderSystem()
{
    m_enableBPred = false;
    m_enableHMPred = false;
    m_enableCache = false;
}

InorderSystem::~InorderSystem()
{}

void InorderSystem::Run( SystemContext* context )
{
    m_enableBPred = context->inorderParam.enableBPred;
    m_enableHMPred = context->inorderParam.enableHMPred;
    m_enableCache = context->inorderParam.enableCache;

    const s64 numInsns = context->executionInsns;
    for( int i = 0; i < context->threads.GetSize(); i++ ){
        g_dumper.SetCurrentCycle( context->threads[i], 0 );
    }

    int processCount = context->emulator->GetProcessCount();

    // レジスタの初期化
    ArchitectureStateList& archStateList = context->architectureStateList;

    EmulatorIF* emulator = context->emulator;
    s64 totalInsnCount = 0;
    vector<s64> insnCount;
    u64 opCount = 0;
    int curPID = 0;
    int terminateProcesses = 0;

    insnCount.resize( processCount, 0 );

    while( totalInsnCount < numInsns ){
        // Decide a thread executed in this iteration.
        PC& curThreadPC = archStateList[curPID].pc;
        curPID = (curPID + 1);  // Round robin
        if( curPID >= processCount ){
            curPID -= processCount;
        }

        std::pair<OpInfo**, int> ops = emulator->GetOp(curThreadPC);

        OpInfo** opInfoArray = ops.first;
        int infoCount        = ops.second;

        // inorder な実行で info が無い領域をフェッチ＝プログラム終了
        if( opInfoArray == 0 || opInfoArray[0] == 0 ) {
            terminateProcesses++;
            if(terminateProcesses >= processCount){
                break;
            }
            else{
                continue;
            }
        }

        totalInsnCount++;
        insnCount[curPID]++;

        Thread* thread = context->threads[ curThreadPC.tid ];
        Core*   core = thread->GetCore();
        InorderList* inorderList = thread->GetInorderList();
        BPred* bPred = core->GetFetcher()->GetBPred();
        RegDepPredIF* regDepPred = thread->GetRegDepPred();
        MemOrderManager* memOrderManager = thread->GetMemOrderManager();
        HitMissPredIF* hmPred = core->GetLatPred()->GetHitMissPred();
        CacheSystem* cacheSystem = core->GetCacheSystem();
        Cache* cache = cacheSystem->GetFirstLevelDataCache();
        MemIF* memImage = emulator->GetMemImage();
        Renamer* renamer = core->GetRenamer();
        DispatchSteererIF* steerer = renamer->GetSteerer();

        // opInfoArray の命令を全て実行
        for(int infoIndex = 0; infoIndex < infoCount; ++infoIndex){
            OpInfo* opInfo  = opInfoArray[infoIndex];
            OpClass opClass = opInfo->GetOpClass();

            OpInitArgs args = 
            {
                // Todo: thread op count
                &curThreadPC, 
                opInfo,
                infoIndex,
                opCount,
                opCount,
                opCount,
                thread->GetCore(), 
                thread
            };
            
            OpIterator op = inorderList->ConstructOp( args );
            
            op->SetTakenPC( Addr(curThreadPC.pid, curThreadPC.tid, curThreadPC.address+4) );

            // ソースオペランドを設定
            regDepPred->Resolve(op);

            // デスティネーションレジスタを設定
            regDepPred->Allocate(op);

            // メモリ命令がフェッチされた
            if( opClass.IsLoad() ) {
                memOrderManager->Allocate(op);
            }

            // 分岐命令がフェッチされた
            if( m_enableBPred && opClass.IsBranch() ) {
                PC predPC = bPred->Predict(op, op->GetPC());

                // BTB/RAS の未初期化エントリ が，違うPIDのものを返してくる
                predPC.pid = op->GetPC().pid;
                predPC.tid = op->GetPC().tid;

                op->SetPredPC(predPC);
            }

            if( !opClass.IsNop() ){
                Scheduler* sched = steerer->Steer( op );
                op->SetScheduler( sched );
                
                ExecUnitIF* execUnit = op->GetExecUnit();
                execUnit->Begin();
                execUnit->Reserve( op, 0 );
            
                // emulation
                op->ExecutionBegin();
                op->SetStatus( OpStatus::OS_EXECUTING );
                op->ExecutionEnd();
            }

            // ロード命令の処理
            if( opClass.IsLoad() ){
                CacheAccess readAccess( op->GetMemAccess(), op, CacheAccess::OT_READ );
                if( m_enableCache ){
                    if( m_enableHMPred ){
                        hmPred->Predict( op );
                    }
                    CacheAccessResult result = cache->Read( readAccess, NULL );
                    op->SetCacheAccessResult( result );

                    if( m_enableHMPred ){
                        bool hit = result.state == CacheAccessResult::ST_HIT;
                        hmPred->Finished( op, hit );
                        hmPred->Commit( op, hit );
                    }
                }

                if( readAccess.result != MemAccess::MAR_SUCCESS ){
                    RUNTIME_WARNING( 
                        "An access violation occurs.\n%s\n%s",
                        readAccess.ToString().c_str(), op->ToString().c_str() 
                    );
                }
            }

            // ストア命令の処理
            if( opClass.IsStore() ) {
                CacheAccess writeAccess( op->GetMemAccess(), op, CacheAccess::OT_WRITE );
                memImage->Write( &writeAccess );

                if( writeAccess.result != MemAccess::MAR_SUCCESS ){
                    RUNTIME_WARNING( 
                        "An access violation occurs.\n%s\n%s",
                        writeAccess.ToString().c_str(), op->ToString().c_str() 
                    );
                }

                if( m_enableCache ){
                    CacheAccessResult result = cache->Write( writeAccess, NULL );
                    op->SetCacheAccessResult( result );
                }
            }
            
            // 分岐命令の処理
            if( m_enableBPred && opClass.IsBranch() ) {
                bPred->Finished(op);
                bPred->Commit(op);
            }

            op->SetStatus( OpStatus::OS_WRITTEN_BACK );

            // 命令の結果をdump
            g_dumper.Dump( DS_FETCH, op );
            g_dumper.Dump( DS_RETIRE, op );

            // メモリ命令がリタイアした
            if( opClass.IsLoad() ) {
                memOrderManager->Commit( op );
                memOrderManager->Retire( op ); // flush: true
            }

            cacheSystem->Commit( op );

            // Commit
            emulator->Commit( &*op, opInfo );

            // レジスタを解放
            regDepPred->Commit(op);

            ++opCount;

            // 次のPCをアップデート
            if ( infoIndex == infoCount - 1 ) {
                if (op->GetTaken()) {
                    curThreadPC = op->GetTakenPC();
                }else {
                    curThreadPC.address += SimISAInfo::INSTRUCTION_WORD_BYTE_SIZE;
                }
            }

            inorderList->Commit( op );
            inorderList->Retire( op );
            inorderList->DestroyOp( op );
        }
    }

    context->executedInsns  = insnCount;
    context->executedCycles = 0;

    // Runに移行するためにセット
    ISAInfoIF* isaInfo = context->emulator->GetISAInfo();
    int logicalRegCount = isaInfo->GetRegisterCount();

    for( int pid = 0; pid < processCount; pid++ ){

        RegDepPredIF* regDepPred = context->threads[pid]->GetRegDepPred();
        RegisterFile* regFile    = context->threads[pid]->GetCore()->GetRegisterFile();

        // register value
        for(int i = 0; i < logicalRegCount; ++i) {
            int phyRegNo = regDepPred->PeekReg( i );
            PhyReg* reg  = regFile->GetPhyReg( phyRegNo );
            archStateList[pid].registerValue[i] = reg->GetVal();
        }
    }

}
