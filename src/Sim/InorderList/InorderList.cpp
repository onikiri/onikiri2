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

#include "Sim/InorderList/InorderList.h"


#include "Sim/Dumper/Dumper.h"

#include "Sim/Foundation/Hook/HookUtil.h"
#include "Sim/Op/Op.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/Core/Core.h"
#include "Sim/Thread/Thread.h"

#include "Sim/Memory/MemOrderManager/MemOrderManager.h"
#include "Sim/Memory/Cache/CacheSystem.h"
#include "Sim/Predictor/DepPred/RegDepPred/RegDepPredIF.h"
#include "Sim/Predictor/DepPred/MemDepPred/MemDepPredIF.h"
#include "Sim/Op/OpObserver/OpObserver.h"

#include "Sim/Pipeline/Fetcher/Fetcher.h"
#include "Sim/Pipeline/Renamer/Renamer.h"
#include "Sim/Pipeline/Dispatcher/Dispatcher.h"
#include "Sim/Pipeline/Retirer/Retirer.h"

#include "Utility/RuntimeError.h"

namespace Onikiri
{
    HookPoint<InorderList> InorderList::s_opFlushHook;
}

using namespace std;
using namespace Onikiri;

InorderList::InorderList() :
    m_core(0),
    m_checkpointMaster(0),
    m_thread(0),
    m_notifier(0),
    m_memOrderManager(0),
    m_regDepPred(0),
    m_memDepPred(0),
    m_fetcher(0),
    m_renamer(0),
    m_dispatcher(0),
    m_retirer(0),
    m_cacheSystem(0),
    m_inorderList(0),
    m_mode( SM_SIMULATION ),
    m_capacity(0),
    m_removeOpsOnCommit(false),
    m_retiredOps(0),
    m_retiredInsns(0)
{

}

InorderList::~InorderList()
{
    ReleaseParam();
}

void InorderList::Initialize(InitPhase phase)
{
    if(phase == INIT_PRE_CONNECTION){
        LoadParam();
    }
    else if(phase == INIT_POST_CONNECTION){
        if( m_core == 0 ) {
            THROW_RUNTIME_ERROR("core not set.");
        }
        if( m_checkpointMaster == 0 ) {
            THROW_RUNTIME_ERROR("CheckpointMaster not set.");
        }
        if( m_thread == 0 ) {
            THROW_RUNTIME_ERROR("thread not set.");
        }

        m_opArray     = m_core->GetOpArray();
        m_notifier    = new OpNotifier();
        
        m_inorderList.resize( *m_opArray );
        m_committedList.resize( *m_opArray );

        m_memOrderManager = m_thread->GetMemOrderManager();
        m_regDepPred = m_thread->GetRegDepPred();
        m_memDepPred = m_thread->GetMemDepPred();

        m_fetcher    = m_core->GetFetcher();
        m_dispatcher = m_core->GetDispatcher();
        m_renamer    = m_core->GetRenamer();
        m_retirer    = m_core->GetRetirer();

        m_cacheSystem = m_core->GetCacheSystem();

        if( m_fetcher->GetFetchWidth() > m_capacity ){
            THROW_RUNTIME_ERROR( "The capacity of InOrderList is too small. It must be larger than fetch width." );
        }
    }
}


// --- Op の生成・破棄に関する関数
// Op の生成
OpIterator InorderList::ConstructOp(const OpInitArgs& args)
{
    OpIterator op = m_opArray->CreateOp();
    op->Initialize(args);

    return op;
}
        
// Op の破棄
void InorderList::DestroyOp(OpIterator op)
{
    m_opArray->ReleaseOp(op);
}


// --- フェッチに関係する関数
// opがfetchされたときの処理
void InorderList::PushBack(OpIterator op)
{
    // 末尾にエントリを作る
    m_inorderList.push_back(op);
}

// --- Remove a back op from a list.
void InorderList::PopBack()
{
    m_inorderList.pop_back();
}


// --- 先頭/末尾の命令やある命令の前/次の命令を得る関数
// 未コミット命令のうち，プログラムオーダで先頭に命令
OpIterator InorderList::GetCommittedFrontOp()
{
    if( m_committedList.empty() ){
        return OpIterator(0);
    }
    
    return m_committedList.front();
}

OpIterator InorderList::GetFrontOp()
{
    if( m_inorderList.empty() ){
        return OpIterator(0);
    }
    
    return m_inorderList.front();
}

// プログラムオーダで最後にある命令
OpIterator InorderList::GetBackOp()
{
    if(m_inorderList.empty())
        return OpIterator(0);
    else
        return m_inorderList.back();
}

// 直前のIndexのOpを返す
/*
op1 ( PC = PC0, No = 0 )
op2 ( PC = PC0, No = 1 )
op3 ( PC = PC1, No = 0 )

GetPrevIndexOp(op3) -> op2
GetPrevIndexOp(op2) -> op1
*/
OpIterator InorderList::GetPrevIndexOp(OpIterator op)
{
    if( GetFrontOp() == op ){
        return OpIterator(0);
    }

    return *(--m_inorderList.get_iterator(op));
}

// 直後のIndexのOpを返す
/*
op1 ( PC = PC0, No = 0 )
op2 ( PC = PC0, No = 1 )
op3 ( PC = PC1, No = 0 )

GetNextIndexOp(op1) -> op2
GetNextIndexOp(op2) -> op3
*/
OpIterator InorderList::GetNextIndexOp(OpIterator op)
{
    if(m_inorderList.back() == op) {
        return OpIterator(0);
    }

    return *(++m_inorderList.get_iterator(op));
}

// 直前のPCのOpを返す
/*
op1 ( PC = PC0, No = 0 )
op2 ( PC = PC0, No = 1 )
op3 ( PC = PC1, No = 0 )
op4 ( PC = PC1, No = 1 )

GetPrevPCOp(op3) -> op2
GetPrevPCOp(op4) -> op2
*/
OpIterator InorderList::GetPrevPCOp(OpIterator op)
{
    int opNo = op->GetNo();
    
    OpIterator prev = op;

    // opNo + 1 回 GetPrevIndexOp を呼ぶと直前のPCの命令が得られる
    // 上の例では op3 だと 1回、op4 だと2回
    for(int i = 0; i <= opNo; ++i) {
        prev = GetPrevIndexOp(prev);
        
        if( prev.IsNull() ) {
            break;
        }
    }
    return prev;
}

// 直後のPCの先頭のOpを返す
/*
op1 ( PC = PC0, No = 0 )
op2 ( PC = PC0, No = 1 )
op3 ( PC = PC1, No = 0 )

GetNextPCOp(op1) -> op3
*/
OpIterator InorderList::GetNextPCOp(OpIterator op)
{

    OpIterator next = GetNextIndexOp(op);

    // 次に No == 0 である命令を探す
    while( (!next.IsNull()) && (next->GetNo() != 0) ) {
        next = GetNextIndexOp(next);
    }

    return next;
}

// 同じPC の先頭のOpを返す(同一 MicroOp の先頭の命令)
/*
op1 ( PC = PC0, No = 0 )
op2 ( PC = PC0, No = 1 )
op3 ( PC = PC1, No = 0 )

GetNextPCOp(op2) -> op1
GetNextPCOp(op3) -> op3
*/
OpIterator InorderList::GetFrontOpOfSamePC(OpIterator op)
{
    OpIterator previousOp = GetPrevPCOp(op);

    if( ! previousOp.IsNull() ) {
        return GetNextIndexOp(previousOp);
    }else {
        return GetFrontOp();
    }
}

// 空かどうか
bool InorderList::IsEmpty()
{
    return m_inorderList.empty() && m_committedList.empty();
}

// Return whether an inorder list can reserve entries or not.
bool InorderList::CanAllocate( int ops )
{
    int current = (int)m_inorderList.size();
    if( !m_removeOpsOnCommit ){
        current += (int)m_committedList.size();
    }
    return ops + current <= m_capacity; 
}


void InorderList::Commit( OpIterator op )
{
    if( m_mode == SM_SIMULATION ){
        ASSERT( 
            m_inorderList.front() == op, 
            "The front of a raw inorder list and the committed op are inconsistent. The committed op:\n%s", 
            op->ToString().c_str() 
        );

        g_dumper.Dump( DS_COMMIT, op );
        NotifyCommit( op );
        op->SetStatus( OpStatus::OS_COMITTING );

        // Move to the committed list.
        m_inorderList.pop_front();
        m_committedList.push_back( op );
    }
}

void InorderList::Retire( OpIterator op )
{
    bool headOp = op->GetNo() == 0; // This op is a head of divided micro ops or not.

    if( m_mode == SM_SIMULATION ){
        g_dumper.Dump( DS_RETIRE, op );
        op->SetStatus( OpStatus::OS_RETIRED );

        for( OpIterator i = m_committedList.front();
             i->GetStatus() == OpStatus::OS_RETIRED;
        ){
            m_committedList.pop_front();
            NotifyRetire( i );
            DestroyOp( i );
            
            if( m_committedList.empty() ){
                break;
            }

            i = m_committedList.front();
        }
    }

    if( headOp ){
        ++m_retiredInsns;
    }
    m_retiredOps++;
}


// Flush all backward ops from 'startOp'.
// This method flushes ops including 'startOp'.
int InorderList::FlushBackward( OpIterator startOp )
{
    // ミスした命令が最後にフェッチされた命令だった場合はなにもしない
    if( startOp.IsNull() ){
        return 0;
    }

    // back() から startOp を発見するまでFlushする
    int flushedInsns = 0;
    while( true ){
        OpIterator op = GetBackOp();
        Flush( op );
        flushedInsns++;
        
        PopBack();

        bool end = ( op == startOp ) ? true : false;

        // op の生存時間はここまで
        DestroyOp( op );

        if( end ){
            break;
        }
    }

    return flushedInsns;
}

void InorderList::Flush(OpIterator op)
{
    HOOK_SECTION_OP( s_opFlushHook, op )
    {
        g_dumper.Dump( DS_FLUSH, op );
        NotifyFlush( op );
        op->SetStatus( OpStatus::OS_FLUSHED );
    }
}

void InorderList::NotifyCommit(OpIterator op)
{
    // チェックポイントを持っていたらコミット
    if( op->GetBeforeCheckpoint() ) {
        m_checkpointMaster->Commit(op->GetBeforeCheckpoint());
    }
    if( op->GetAfterCheckpoint() ) {
        m_checkpointMaster->Commit(op->GetAfterCheckpoint());
    }

    // CacheSystem must be retired after that of MemOrderManager,
    // because CacheSystem uses cache access results of stores 
    // set by MemOrderManager.
    m_memOrderManager->Commit( op );
    m_cacheSystem->Commit( op );    

    m_regDepPred->Commit(op);
    m_memDepPred->Commit(op);

    m_fetcher->Commit(op);
    m_dispatcher->Commit(op);
    m_renamer->Commit(op);
    if( op->GetScheduler() != 0 ) {
        op->GetScheduler()->Commit(op);
    }

    m_notifier->NotifyCommit( op );
}


void InorderList::NotifyRetire( OpIterator op )
{
    op->ClearEvent();

    if( op->GetScheduler() ){
        op->GetScheduler()->Retire( op );
    }

    m_cacheSystem->Retire( op );
    m_memOrderManager->Retire( op );
    m_dispatcher->Retire( op );
    m_renamer->Retire( op );
    m_fetcher->Retire( op );

    // src の dependency にいなくなることを伝える
    op->DissolveSrcReg();
    op->DissolveSrcMem();
}

void InorderList::NotifyFlush( OpIterator op )
{
    if( op->GetAfterCheckpoint() ){
        m_checkpointMaster->Flush( op->GetAfterCheckpoint() );
    }
    if( op->GetBeforeCheckpoint() ){
        m_checkpointMaster->Flush( op->GetBeforeCheckpoint() );
    }

    op->CancelEvent();
    op->ClearEvent();
    if( op->GetScheduler() ){
        op->GetScheduler()->Flush( op );
    }

    m_cacheSystem->Flush( op );
    m_memOrderManager->Flush( op );
    m_retirer->Flush( op );
    m_dispatcher->Flush( op );
    m_renamer->Flush( op );
    m_fetcher->Flush( op );
    m_regDepPred->Flush(op);
    m_memDepPred->Flush(op);

    op->DissolveSrcReg();   // src の dependency にいなくなることを伝える
    op->DissolveSrcMem();

    m_notifier->NotifyFlush( op );
}
