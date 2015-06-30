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

#include "Sim/Predictor/LatPred/LatPred.h"
#include "Sim/Predictor/HitMissPred/CounterBasedHitMissPred.h"
#include "Sim/Predictor/HitMissPred/StaticHitMissPred.h"
#include "Sim/ExecUnit/ExecUnitIF.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/Op/Op.h"
#include "Sim/Core/Core.h"

using namespace Onikiri;

namespace Onikiri
{
    HookPoint<LatPred> LatPred::s_latencyPredictionHook;
}; // Onikiri

LatPred::LatPred() :
    m_execLatencyInfo(0),
    m_hmPredictor(0),
    m_core(0),
    m_numHit(0),
    m_numMiss(0),
    m_numLoadHitPredToHit(0),
    m_numLoadHitPredToMiss(0),
    m_numLoadMissPredToHit(0),
    m_numLoadMissPredToMiss(0)
{
}

LatPred::~LatPred()
{
    ReleaseParam();
}

void LatPred::Initialize(InitPhase phase)
{
    if(phase == INIT_PRE_CONNECTION){
        LoadParam();
    }
    else if(phase == INIT_POST_CONNECTION){
        CheckNodeInitialized( "execLatencyInfo", m_execLatencyInfo );
        CheckNodeInitialized( "hmPredictor", m_hmPredictor );
    }
}

// レイテンシ予測
// 現在2レベル（L1/L2）の予測にのみ対応
// L1ミス後は，常にL2ヒットと予測される
void LatPred::Predict(OpIterator op)
{
    const OpClass& opClass = op->GetOpClass();

    LatPredResult result;

    s_latencyPredictionHook.Trigger(op, this, HookType::HOOK_BEFORE);

    if( !s_latencyPredictionHook.HasAround() ) {
        // 本来の処理
        // Load/store hit miss prediction
        // レイテンシ予測とスケジューリングのモデルについてはこのファイルの末尾を参照
        ExecUnitIF* execUnit = op->GetExecUnit();
        int latencyCount = execUnit->GetLatencyCount(opClass);
        
        if( opClass.IsLoad() ){

            static const int MAX_PRED_LEVEL = LatPredResult::MAX_COUNT;
            ASSERT( latencyCount <= MAX_PRED_LEVEL, "Make MAX_PRED_LEVEL large enough." );
            result.SetCount( latencyCount ); 

            // Get latencies
            int latencies[ MAX_PRED_LEVEL ];
            for( int i = 0; i < latencyCount; ++i ){
                latencies[i] = execUnit->GetLatency( opClass, i );
            }
            
            // L1 prediction
            bool lv1Hit = m_hmPredictor->Predict( op );
            result.Set( 0, latencies[0], lv1Hit );
            
            // Issue latency
            int issueLatency = op->GetScheduler()->GetIssueLatency();

            // Lv2 & higher caches are predicted as always hit.
            bool prevLevelWokeup = lv1Hit;
            for( int i = 1; i < latencyCount; ++i ){

                // 0  1  2  3  4  5  6  7  8                  
                // SC IS IS IS IS L1 L2 L2 L2
                //             SC IS IS IS IS EX
                //                ^^ L1 miss detected point
                //                   SC IS IS IS IS EX
                // detected     : 1(SC) + 4(IS) + 1(L1)
                // second issue: 1(SC) + L2(3)
                // second issue - detected = L2 - IS - L1

                int wakeupTime = latencies[i];
                if( prevLevelWokeup ){
                    int detectTime = issueLatency + latencies[i - 1] + 1 + 1;// + rescheduling + select
                    if( wakeupTime < detectTime ){
                        wakeupTime = detectTime;
                    }
                }
                result.Set( i, wakeupTime, true );
                prevLevelWokeup = true;
            }

            // A last level memory is predicted as always miss and does not wakeup.
            if( latencyCount > 1 ){
                result.Set( 
                    latencyCount - 1, 
                    latencies[latencyCount - 1], 
                    false 
                );
            }


        }   // if(opClass.IsLoad()) {
        else{
            
            ASSERT( latencyCount == 1, "A variable latency is not supported except case of Load." );

            int latency = execUnit->GetLatency( opClass, 0 );

            result.SetCount( 1 );
            result.Set( 0, latency, true );
        }

        op->SetLatPredRsult( result );
    }
    else {
        s_latencyPredictionHook.Trigger(op, this, HookType::HOOK_AROUND);
    }

    s_latencyPredictionHook.Trigger(op, this, HookType::HOOK_AFTER);

}

//
// predictionHit : レイテンシ予測の結果が的中したかどうか(L1ヒットの意味ではない)
//
void LatPred::Commit( OpIterator op )
{
    if( !op->GetOpClass().IsLoad() ){
        return;
    }

    int latency = op->GetIssueState().executionLatency;
    const LatPredResult& latPredResult = 
        op->GetLatPredRsult();
    const LatPredResult::Scheduling& predSched = latPredResult.Get(0);
    
    bool predictedAsHitLv1 = predSched.wakeup;
    bool hitMissOfLV1      = latency == predSched.latency ? true : false;
    if( hitMissOfLV1 ){
        if( predictedAsHitLv1 ){
            m_numLoadHitPredToHit++;
        } 
        else{
            m_numLoadHitPredToMiss++;
        }
    }
    else{
        if( predictedAsHitLv1 ){
            m_numLoadMissPredToHit++;
        } else {
            m_numLoadMissPredToMiss++;
        }
    }

    if( predictedAsHitLv1 == hitMissOfLV1 )
        m_numHit++;
    else
        m_numMiss++;

    m_hmPredictor->Commit( op, hitMissOfLV1 );
}

void LatPred::Finished( OpIterator op )
{
    if( !op->GetOpClass().IsLoad() ){
        return;
    }

    int latency = op->GetIssueState().executionLatency;
    const LatPredResult::Scheduling& predSched = op->GetLatPredRsult().Get(0);
    bool hitMissOfLV1      = latency == predSched.latency ? true : false;
    m_hmPredictor->Finished( op, hitMissOfLV1 );
}

/*

--- レイテンシ予測とスケジューリングのモデル

レイテンシ予測を行う命令(Ip:producer)と，予測を行った命令に
依存する命令(Ic:consumer)をどのようにスケジュールするか．

例：
    Ip: load r1 = [A]
    Ic: add  r2 = r1 + 4

今，Ip がキャッシュにミスすると予測した場合を考える
多くの論文ではIp とIc は以下のようにスケジュールされるとしている

    time: 0  1  2  3  4  5  6  7  8
    --------------------------------
    Ip:   S  I  R  1  2  2  2  W
    Ic:   S  S  S  S  S  I  R  X  W

    S : Scheduling
    R : Register Read
    1 : L1 access
    2 : L2 access
    X : execute
    W : Register Write

    Issue latency : 1cycle
    L1 latency    : 1cycle
    L2 latency    : 3cycle


レイテンシ予測を行った命令がいつ実行終了するか（ヒット/ミスの判明）は
基本的に実行終了時まで不明である．
Ip が6cycle 目以降のフォワーディングポート、7cycle 目のレジスタ
書き込みポートを利用するためには，ずっとポートを予約
しておかなくてはならないが、このような実装は妥当とは考えにくい．
（ポートを予約しておかないと，その時発行されてきた他の命令と資源競合を起こす．

実際のハードウェアでは，以下のようにミスすると予測した場合には
Ip を2回発行を行う実装になっていると考えられる．
以下の例では，Ip0 でミスが判明した3サイクル目以降にL2 のアクセスを開始し，
ちょうどL1 にデータが取れてくるタイミングで2回目のIp1 を発行する．
Ic は，Ip1 の実行後にあわせる形で発行を行う．

    time: 0  1  2  3  4  5  6  7  8  9
    -----------------------------------
    Ip0:  S  I  R  1 (2  2  2) 
    Ip1:  .  .  .  .  S  I  R  1  W
    Ic:   S  S  S  S  S  S  I  R  X  W

*/
