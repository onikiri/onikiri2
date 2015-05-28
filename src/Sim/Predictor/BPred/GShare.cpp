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

#include "Sim/Predictor/BPred/GShare.h"

#include "Sim/Dumper/Dumper.h"
#include "Sim/ISAInfo.h"
#include "Sim/Thread/Thread.h"
#include "Sim/Core/Core.h"
#include "Sim/Op/Op.h"

#include "Sim/Predictor/BPred/BTB.h"
#include "Sim/Predictor/BPred/PHT.h"
#include "Sim/Predictor/BPred/GlobalHistory.h"

using namespace Onikiri;

GShare::GShare()
{
    m_core = 0;
    m_pht = 0;

    m_globalHistoryBits = 0;
    m_pthIndexBits = 0;

    m_numPred   =   0;
    m_numHit    =   0;
    m_numMiss   =   0;
    m_numRetire =   0;

}

GShare::~GShare()
{
    ReleaseParam();
}

void GShare::Initialize(InitPhase phase)
{
    if(phase == INIT_PRE_CONNECTION){
        LoadParam(); // m_jBitSize, m_kBitSizeの初期化
    }
    else if(phase == INIT_POST_CONNECTION){
        // メンバ変数が正しく初期化されているかのチェック
        CheckNodeInitialized( "core", m_core);
        CheckNodeInitialized( "globalHistory", m_globalHistory);
        CheckNodeInitialized( "pht", m_pht);

        // table の初期化
        m_predTable.Resize( *m_core->GetOpArray() );
        
        m_pthIndexBits = m_pht->GetIndexBitSize();
        if(m_pthIndexBits < m_globalHistoryBits){
            THROW_RUNTIME_ERROR("PHT size is less than global history size.");
        }
    }
}

// 分岐の方向を予測
bool GShare::Predict(OpIterator op, PC predIndexPC)
{
    ASSERT(
        op->GetTID() == predIndexPC.tid,
        "The tread ids of the op and current pc are different."
    );

    ++m_numPred;
    int localTID = op->GetLocalTID();
    int phtIndex = GetPHTIndex( localTID, predIndexPC );
    bool taken   = m_pht->Predict(phtIndex);
    m_globalHistory[localTID]->Predicted(taken);

    PredInfo& info = m_predTable[op];

    // 更新のために pht のindex を覚えておく
    info.phtIndex  = phtIndex;

    // Hit/Missの判定のために、予測した方向を覚えておく
    info.direction = taken;

    return taken;
}

//
// 実行完了
// 再実行で複数回呼ばれる可能性がある
// また、間違っている結果を持っている可能性もある
//
// opの実行完了時にPHTのUpdateを行う
//
void GShare::Finished(OpIterator op)
{
    PredInfo& info = m_predTable[op];
    bool taken = op->GetTaken();
    m_pht->Update(info.phtIndex, taken);

    // 予測Miss時に強制的にGlobalHistoryの最下位ビットを変更する
    // GShare::Finishedが呼ばれるときにはすでにチェックポイントの巻き戻しが
    // 終わっているので最下位ビットはミスした分岐に対応したビットになっている
    if(info.direction != taken){
        m_globalHistory[op->GetLocalTID()]->SetLeastSignificantBit(taken);
    }
}

// opのretire時の動作
void GShare::Retired(OpIterator op)
{
    bool taken = op->GetTaken();

    // CheckpointがGlobalHistoryをCommitするのでこの関数の中では何もupdateしない
    m_globalHistory[op->GetLocalTID()]->Retired( taken ); 

    // 予測Hit/Missの判定
    if(m_predTable[op].direction == taken){
        ++m_numHit;
    } 
    else{
        ++m_numMiss;
    }

    ++m_numRetire;
}

// PCに対応するPHTのインデックスを返す
int GShare::GetPHTIndex(int localThreadID, const PC& pc)
{
    // pc の下位ビットを切り捨て
    u64 p = pc.address >> SimISAInfo::INSTRUCTION_WORD_BYTE_SHIFT;

    // p から jBit + kBit 幅を切り出す
    if(m_addrXORConvolute){
        p = shttl::xor_convolute(p, m_pthIndexBits);
    }
    else{
        p = p & shttl::mask(0, m_pthIndexBits);
    }

    int pos    = m_pthIndexBits - m_globalHistoryBits;
    u64 mask   = shttl::mask(0, m_globalHistoryBits);
    u64 global = (m_globalHistory[localThreadID]->GetHistory() & mask) << pos;
    return (int)( p ^ global );
}
