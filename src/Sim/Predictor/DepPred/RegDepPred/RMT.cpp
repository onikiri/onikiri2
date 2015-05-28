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

#include "Sim/Predictor/DepPred/RegDepPred/RMT.h"
#include "Sim/Foundation/Checkpoint/CheckpointMaster.h"
#include "Interface/ISAInfo.h"
#include "Sim/Op/Op.h"
#include "Sim/Foundation/Hook/HookUtil.h"



using namespace Onikiri;
using namespace std;
using namespace boost;

namespace Onikiri
{
    HookPoint<RMT,RMT::HookParam> RMT::s_allocateRegHook;
    HookPoint<RMT,RMT::HookParam> RMT::s_releaseRegHook;
    HookPoint<RMT,RMT::HookParam> RMT::s_deallocateRegHook;
};  // namespace Onikiri


// RMT の初期化前に RegisterFile / CheckpointMaster の初期化が行われている必要がある
RMT::RMT() :
    m_numLogicalReg(0),
    m_numRegSegment(0),
    m_core(0),
    m_checkpointMaster(0),
    m_emulator(0),
    m_registerFile(0),
    m_regFreeList(0),
    m_allocationTable()
{
}

RMT::~RMT()
{
    ReleaseParam();
}

void RMT::Initialize(InitPhase phase)
{
    if(phase == INIT_POST_CONNECTION){
        // メンバ変数のチェック
        CheckNodeInitialized( "registerFile",   m_registerFile );
        CheckNodeInitialized( "regFreeList",    m_regFreeList );
        CheckNodeInitialized( "emulator",       m_emulator );
        CheckNodeInitialized( "checkpointMaster", m_checkpointMaster );

        ISAInfoIF* isaInfo = m_emulator->GetISAInfo();

        m_numLogicalReg = isaInfo->GetRegisterCount();
        m_numRegSegment = isaInfo->GetRegisterSegmentCount();

        // checkpointedData の初期化
        CheckpointMaster* checkpointMaster = m_checkpointMaster;
        m_allocationTable.Initialize(
            checkpointMaster,
            CheckpointMaster::SLOT_RENAME
        );

        // セグメント情報をテーブルに格納
        m_segmentTable.assign(-1);
        for (int i = 0; i < m_numLogicalReg; ++i) {
            int segment = isaInfo->GetRegisterSegmentID(i);
            m_segmentTable[i] = segment;
        }
        vector<int> logicalRegNum( m_numRegSegment , 0 );
        
        // 論理レジスタの数だけレジスタを初期化
        for (int i = 0; i < m_numLogicalReg; i++) {
            // freelist から1つレジスタ番号をもらう
            int segment = GetRegisterSegmentID(i);
            int phyRegNo = m_regFreeList->Allocate(segment);
            logicalRegNum[segment]++;

            // 論理レジスタ→物理レジスタの割り当て
            (*m_allocationTable)[i] = phyRegNo;

            // レジスタの状態の初期化
            // ここで初期化されなかったものはallocateされる時に初期化される
            PhyReg* phyReg = (*m_registerFile)[phyRegNo];
            phyReg->Clear();

            // 一番初めは全部レディ
            phyReg->Set();
            // value を m_emulatorから受け取る
            phyReg->SetVal(m_emulator->GetInitialRegValue(0, i));
        }

        // 解放用のテーブルの初期化
        // 初期状態ではデスティネーションレジスタとして割り当てられた物理レジスタはいない
        // 
        m_releaseTable.resize(m_registerFile->GetTotalCapacity(), -1);
    }
}

// マップ表をひいて論理レジスタ番号に対応する物理レジスタの番号を返す
int RMT::ResolveReg(int lno)
{
    // RMT の場合，ResolveReg で副作用は生じないため，
    // PeekReg に委譲
    return PeekReg(lno);
}

// ResolveReg と同様に物理レジスタ番号を返す．
// ただし，副作用がないことが保証される
int RMT::PeekReg(int lno) const
{
    ASSERT(
        lno >= 0 && lno < m_numLogicalReg,
        "illegal register No.: %d\n", lno
    );

    return ( m_allocationTable.GetCurrent() )[lno];
}


// フリーリストから割り当て可能な物理レジスタの番号を受け取る
int RMT::AllocateReg( OpIterator op, int lno )
{
    HookParam hookParam = {op, lno, 0};
    HookEntry(
        this,
        &RMT::AllocateRegBody,
        &s_allocateRegHook,
        &hookParam 
    );
    return hookParam.physicalRegNum;
}

// Retireしたので、値が利用されることのなくなる物理レジスタを解放する
void RMT::ReleaseReg( OpIterator op, const int lno, int phyRegNo )
{
    HookParam hookParam = { op, lno, phyRegNo };
    HookEntry(
        this,
        &RMT::ReleaseRegBody,
        &s_releaseRegHook,
        &hookParam 
    );
}

// Flushされたのでopのデスティネーション・レジスタをフリーリストに戻す
void RMT::DeallocateReg( OpIterator op, const int lno, int phyRegNo )
{
    HookParam hookParam = { op, lno, phyRegNo };
    HookEntry(
        this,
        &RMT::DeallocateRegBody,
        &s_deallocateRegHook,
        &hookParam 
    );
}


// AllocateRegの実装
void RMT::AllocateRegBody( HookParam* param )
{
    int lno = param->logicalRegNum;
    int segment = GetRegisterSegmentID( lno );

    ASSERT( 
        lno >= 0 && lno < m_numLogicalReg,
        "illegal register No.: %d\n", lno
    );

    // フリーリストから物理レジスタ番号を受け取る
    int phyRegNo = m_regFreeList->Allocate( segment );

    // allocation時に物理レジスタを初期化
    PhyReg* phyReg = (*m_registerFile)[ phyRegNo ];
    phyReg->Clear();

    // regがcommit時に解放する物理レジスタをm_releaseTableに登録
    // それまでregの論理レジスタに割り当てられていた物理レジスタを解放する
    m_releaseTable[ phyRegNo ] = ( m_allocationTable.GetCurrent() )[ lno ];

    // <論理レジスタ、物理レジスタ>のマッピングテーブルを更新
    ( m_allocationTable.GetCurrent() )[ lno ] = phyRegNo;

    param->physicalRegNum = phyRegNo;
}

// Retireしたので、値が利用されることのなくなる物理レジスタを解放する
void RMT::ReleaseRegBody( HookParam* param )
{
    int lno = param->logicalRegNum;
    ASSERT(
        lno >= 0 && lno < m_numLogicalReg,
        "illegal register No.: %d\n", lno 
    );
    
    int segment = GetRegisterSegmentID( lno );

    // 解放する物理レジスタをフリーリストに戻す
    int releasedPhyReg = m_releaseTable[ param->physicalRegNum ];
    m_regFreeList->Release( segment, releasedPhyReg );
    param->physicalRegNum = releasedPhyReg;
}

// Flushされたのでopのデスティネーション・レジスタをフリーリストに戻す
void RMT::DeallocateRegBody( HookParam* param )
{
    int lno = param->logicalRegNum;
    ASSERT(
        lno >= 0 && lno < m_numLogicalReg,
        "illegal register No.: %d\n", lno 
    );

    int segment = GetRegisterSegmentID( lno );

    m_regFreeList->Release( segment, param->physicalRegNum );
}


bool RMT::CanAllocate(OpIterator* infoArray, int numOp)
{
    // 必要なレジスタ数をセグメントごとに数える
    boost::array<size_t, SimISAInfo::MAX_REG_SEGMENT_COUNT> requiredRegCount;
    requiredRegCount.assign(0);

    for(int i = 0; i < numOp; ++i) {
        OpInfo* opInfo = infoArray[i]->GetOpInfo();

        int dstNum = opInfo->GetDstNum();
        for( int k = 0; k < dstNum; ++k ){
            int dstOperand = opInfo->GetDstOperand( k );
            int segment = GetRegisterSegmentID( dstOperand );
            ++requiredRegCount[segment];
        }
    }

    // セグメントごとに割り当て可能か判断
    for(int m = 0; m < m_numRegSegment; ++m){
        size_t freeRegCount = (size_t)m_regFreeList->GetFreeEntryCount(m);
        if( freeRegCount < requiredRegCount[m] ) {
            return false;
        }
    }
    return true;
}

// 論理/物理レジスタの個数
int RMT::GetRegSegmentCount()
{
    return m_numRegSegment;
}

int RMT::GetLogicalRegCount(int segment)
{
    if( segment >= m_numRegSegment )
        return 0;

    return m_segmentTable[segment];
}

int RMT::GetTotalLogicalRegCount()
{
    return m_numLogicalReg;
}
