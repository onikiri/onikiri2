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


//
// GShareの実装
// PHTとGlobalHistoryを持ち、それぞれの更新を予測時/に行う
// 予測ミス時にはCheckpointによって自動的に更新結果が巻き戻される
//

#ifndef __GSHARE_H__
#define __GSHARE_H__

#include "Sim/Predictor/BPred/DirPredIF.h"
#include "Sim/Op/OpContainer/OpExtraStateTable.h"

#include "Env/Param/ParamExchange.h"
#include "Sim/Foundation/Resource/ResourceNode.h"

namespace Onikiri 
{
    class Core;
    class PHT;
    class GlobalHistory;

    class GShare : 
        public PhysicalResourceNode,
        public DirPredIF
    {
    private:
        // Parameter
        int m_globalHistoryBits;
        int m_pthIndexBits;
        bool m_addrXORConvolute;    // Calculate pht index using xor-convolution.

        // 
        Core* m_core;
        PHT*    m_pht;          // PHT
        PhysicalResourceArray<GlobalHistory> 
            m_globalHistory;    // GlobalHistory

        // Prediction information
        struct PredInfo
        {
            // PHT index used for updating PHT
            int  phtIndex;

            // predicted direction
            bool direction;
        };
        OpExtraStateTable<PredInfo> m_predTable;

        // statistical information
        s64 m_numPred;
        s64 m_numHit;
        s64 m_numMiss;
        s64 m_numRetire;

    public:
        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY("@GlobalHistoryBits",   m_globalHistoryBits)
                PARAM_ENTRY("@AddressXORConvolute", m_addrXORConvolute)
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
                PARAM_ENTRY( "@NumPred",        m_numPred)
                PARAM_ENTRY( "@NumHit",     m_numHit)
                PARAM_ENTRY( "@NumMiss",        m_numMiss)
                PARAM_ENTRY( "@NumRetire",  m_numRetire)
                RESULT_RATE_SUM_ENTRY( "@HitRate", \
                    m_numHit, m_numHit, m_numMiss )
            END_PARAM_PATH()
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( Core, "core", m_core )
            RESOURCE_ENTRY( PHT,  "pht",  m_pht )
            RESOURCE_ENTRY( GlobalHistory, "globalHistory", m_globalHistory )
        END_RESOURCE_MAP()

        GShare();
        virtual ~GShare();

        // 初期化
        void Initialize(InitPhase phase);

        // 分岐の方向を予測
        bool Predict(OpIterator op, PC predIndexPC);

        //
        // 実行完了
        // 再実行で複数回呼ばれる可能性がある
        // また、間違っている結果を持っている可能性もある
        //
        // opの実行完了時にPHTのUpdateを行う
        void Finished(OpIterator op);

        // opのretire時の動作
        void Retired(OpIterator op);

        // PCに対応するPHTのインデックスを返す
        int GetPHTIndex(int localThreadID, const PC& pc);

    };

}; // namespace Onikiri

#endif // __GSHARE_H__

