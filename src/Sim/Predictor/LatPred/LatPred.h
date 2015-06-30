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


#ifndef SIM_PREDICTOR_LAD_PRED_LAD_PRED_H
#define SIM_PREDICTOR_LAD_PRED_LAD_PRED_H

#include "Sim/Predictor/LatPred/LatPredResult.h"
#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Op/OpContainer/OpExtraStateTable.h"
#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/ExecUnit/ExecLatencyInfo.h"
#include "Sim/Foundation/Hook/Hook.h"
#include "Sim/Predictor/HitMissPred/HitMissPredIF.h"

namespace Onikiri 
{
    class Core;
    
    // レイテンシ予測とスケジューリングのモデルについてはLatPred.cppの末尾を参照
    // Latency predictor
    class LatPred :
        public PhysicalResourceNode
    {
    public:
        BEGIN_PARAM_MAP( GetResultPath() )
            PARAM_ENTRY( "@NumHit",     m_numHit )
            PARAM_ENTRY( "@NumMiss",    m_numMiss )
            RESULT_RATE_SUM_ENTRY( 
                "@PredictionHitRate", m_numHit, m_numHit, m_numMiss
            )
            
            PARAM_ENTRY( "@NumLoadHitPredToHit",    m_numLoadHitPredToHit )
            PARAM_ENTRY( "@NumLoadHitPredToMiss",   m_numLoadHitPredToMiss )
            RESULT_RATE_SUM_ENTRY(
                "@PredictionHitRatePredToHit", 
                m_numLoadHitPredToHit, m_numLoadHitPredToHit, m_numLoadMissPredToHit
            )   // L1ヒットと予測した予測の的中率
            
            PARAM_ENTRY("@NumLoadMissPredToHit",    m_numLoadMissPredToHit )    
            PARAM_ENTRY("@NumLoadMissPredToMiss",   m_numLoadMissPredToMiss )
            RESULT_RATE_SUM_ENTRY(
                "@PredictionHitRatePredAsMiss",
                m_numLoadMissPredToMiss, m_numLoadHitPredToMiss, m_numLoadMissPredToMiss
            )   // L1ミスと予測した予測の的中率
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( ExecLatencyInfo, "execLatencyInfo", m_execLatencyInfo )
            RESOURCE_ENTRY( HitMissPredIF, "hitMissPred",     m_hmPredictor)
            RESOURCE_ENTRY( Core, "core",     m_core)
        END_RESOURCE_MAP()

        LatPred();
        ~LatPred();
        void Initialize( InitPhase phase );
        void Predict( OpIterator op );
        void Finished( OpIterator op );
        void Commit( OpIterator op );
        HitMissPredIF* GetHitMissPred() { return m_hmPredictor; }

        static HookPoint<LatPred> s_latencyPredictionHook;

    protected:
        // 命令セットで規定されている実行レイテンシ
        ExecLatencyInfo* m_execLatencyInfo;
        // キャッシュHit/Miss予測器
        HitMissPredIF* m_hmPredictor;

        Core* m_core;

        u64 m_numHit;                   // Prediction is hit.
        u64 m_numMiss;                  // Prediction is miss.
        u64 m_numLoadHitPredToHit;      // A load is predicted to hit  and it hit.
        u64 m_numLoadHitPredToMiss;     // A load is predicted to miss and it hit.
        u64 m_numLoadMissPredToHit;     // A load is predicted to hit  and it missed.
        u64 m_numLoadMissPredToMiss;    // A load is predicted to miss  and it missed.


    };

}; // namespace Onikiri

#endif // SIM_PREDICTOR_LAD_PRED_LAD_PRED_H

