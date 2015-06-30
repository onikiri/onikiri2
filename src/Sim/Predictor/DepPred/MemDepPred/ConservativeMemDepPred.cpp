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

#include "Sim/Predictor/DepPred/MemDepPred/ConservativeMemDepPred.h"
#include "Sim/Op/Op.h"
#include "Sim/Core/Core.h"


using namespace std;
using namespace boost;
using namespace Onikiri;

ConservativeMemDepPred::ConservativeMemDepPred() :
    m_core(0),
    m_checkpointMaster(0),
    m_latestStoreDst(),
    m_latestMemDst()
{
}

ConservativeMemDepPred::~ConservativeMemDepPred()
{
    ReleaseParam();
}

void ConservativeMemDepPred::Initialize(InitPhase phase)
{
    if(phase == INIT_POST_CONNECTION){
        // checkpointMaster がセットされているかのチェック
        CheckNodeInitialized( "checkpointMaster", m_checkpointMaster );

        m_latestStoreDst.Initialize(
            m_checkpointMaster,
            CheckpointMaster::SLOT_RENAME
        );
        m_latestMemDst.Initialize(
            m_checkpointMaster,
            CheckpointMaster::SLOT_RENAME
        );
        
        MemDependencyPtr
            tmpStoreDst( 
                m_memDepPool.construct( m_core->GetNumScheduler() )
            );
        tmpStoreDst->Set();
        m_latestStoreDst.GetCurrent() = tmpStoreDst;

        MemDependencyPtr
            tmpMemDst(
                m_memDepPool.construct( m_core->GetNumScheduler() )
            );
        tmpMemDst->Set();
        m_latestMemDst.GetCurrent() = tmpMemDst;
    }
}

// アドレス一致/不一致予測
// load 命令は 最新のstoreに依存すると予測する
// store 命令は 最新の load/store に依存すると予測する
void ConservativeMemDepPred::Resolve(OpIterator op)
{
    // memory命令でなければ終了
    if( !op->GetOpClass().IsMem() ) {
        return;
    }

    if( op->GetOpClass().IsLoad() ) {
        if( m_latestStoreDst.GetCurrent() == NULL ) {
            return;
        }

        op->SetSrcMem(0, m_latestStoreDst.GetCurrent());
    
    }else if( op->GetOpClass().IsStore() ) {
        if( m_latestMemDst.GetCurrent() == NULL ) {
            return;
        }

        op->SetSrcMem(0, m_latestMemDst.GetCurrent());

    }else {
        THROW_RUNTIME_ERROR("Unknown Memory Instruction");
    }
}

// 自分が最後にfetchされたload/storeであることを登録する
void ConservativeMemDepPred::Allocate(OpIterator op)
{
    // memory命令でなければ終了
    if( !op->GetOpClass().IsMem() ) {
        return;
    }

    // op の dstMemにMemDependencyを割り当てる
    if( op->GetDstMem(0) == NULL ) {
        MemDependencyPtr tmpMem(
            m_memDepPool.construct(m_core->GetNumScheduler()) );
        tmpMem->Clear();
        op->SetDstMem(0, tmpMem);
    }

    *m_latestMemDst = op->GetDstMem(0);
    if (op->GetOpClass().IsStore()) {
        *m_latestStoreDst = op->GetDstMem(0);
    }
}

// 最後に登録された依存が自分自身である場合は解放する
void ConservativeMemDepPred::Deallocate(OpIterator op)
{
    MemDependencyPtr dstMem = op->GetDstMem(0);
    if(*m_latestStoreDst == dstMem) {
        m_latestStoreDst->reset();
    }

    if(*m_latestMemDst == dstMem) {
        m_latestMemDst->reset();
    }
}

void ConservativeMemDepPred::Commit(OpIterator op)
{
    if( op->GetOpClass().IsMem() ) {
        Deallocate(op);
    }
}

void ConservativeMemDepPred::Flush(OpIterator op)
{
    if( op->GetStatus() == OpStatus::OS_FETCH ){
        return;
    }
    if( op->GetOpClass().IsMem() ) {
        Deallocate(op);
    }
}

// MemOrderManagerによって、MemOrderのconflictを起こしたopの組(producer, consumer)を教えてもらう
// ConservativeMemDepPredでは何もしない
void ConservativeMemDepPred::OrderConflicted(OpIterator producer, OpIterator consumer)
{
    THROW_RUNTIME_ERROR("Access order violation must not occur with Conservative MemDepPred");
}

// 実際にはPhyRegでは無くメモリ上の依存であるため、物理レジスタの割り当ては行われない
// そのため、必ずtrueを返す
bool ConservativeMemDepPred::CanAllocate(OpIterator* infoArray, int numOp)
{
    return true;
}
