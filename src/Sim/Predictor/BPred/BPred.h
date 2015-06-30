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


#ifndef SIM_PREDICTOR_BPRED_BPRED_H
#define SIM_PREDICTOR_BPRED_BPRED_H

#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Env/Param/ParamExchange.h"

#include "Interface/Addr.h"

#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Op/OpContainer/OpExtraStateTable.h"
#include "Sim/Predictor/BPred/BTB.h"
#include "Sim/Foundation/Hook/HookDecl.h"


namespace Onikiri
{
    class Core;
    class Thread;
    class DirPredIF;
    class GlobalHistory;
    class RAS;
    class ForwardEmulator;

    // ï™äÚó\ë™ëSëÃÇíSìñÇ∑ÇÈÉNÉâÉX
    // FetcherÇ…nextPCÇï‘Ç∑
    class BPred : public PhysicalResourceNode
    {
    public:

        BEGIN_PARAM_MAP( "" )
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY( "@Perfect", m_perfect )
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
                CHAIN_PARAM_MAP( "Statistics", m_totalStatistics )
                CHAIN_PARAM_MAP( "Statistics/Entry", m_statistics )
            END_PARAM_PATH()
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( Core, "core", m_core )
            RESOURCE_ENTRY( BTB,  "btb",  m_btb )
            RESOURCE_ENTRY( RAS,  "ras",  m_ras )
            RESOURCE_ENTRY( DirPredIF, "dirPred", m_dirPred )
            RESOURCE_ENTRY( ForwardEmulator, "forwardEmulator", m_fwdEmulator )
        END_RESOURCE_MAP()

        BPred();
        virtual ~BPred();

        void Initialize( InitPhase phase );

        // opÇÃï™äÚÇ…ëŒÇµÇƒï™äÚó\ë™ÇçsÇ¢ÅCó\ë™Ç≥ÇÍÇΩï™äÚêÊÇï‘Ç∑
        //   predIndexPC : ï™äÚó\ë™Ç…ópÇ¢ÇÈPCÅDí èÌFetch GroupÇÃPC
        PC Predict( OpIterator op, PC predIndexPC );
        void Finished( OpIterator op );
        void Commit( OpIterator op );

        // Is in a perfect prediction mode or not.
        bool IsPerfect() const { return m_perfect; }

        // --- PhysicalResourceNode
        virtual void ChangeSimulationMode( SimulationMode mode ){ m_mode = mode; };

        static HookPoint<BPred> s_branchPredictionMissHook;

    protected:

        DirPredIF*                  m_dirPred;      // ï˚å¸ó\ë™äÌ
        BTB*                        m_btb;          // BTB
        PhysicalResourceArray<RAS>  m_ras;          // RAS
        Core*                       m_core;
        ForwardEmulator*            m_fwdEmulator;
        
        // Simulation mode.
        SimulationMode m_mode;
        
        // Enable perfect branch prediction.
        bool m_perfect; 

        OpExtraStateTable<BTBPredict> m_btbPredTable;

        // Result
        struct Statistics : public ParamExchangeChild
        {
        public:
        
            BEGIN_PARAM_MAP("")
                PARAM_ENTRY("@Name",        name)
                PARAM_ENTRY("@NumHit",      numHit)
                PARAM_ENTRY("@NumMiss",     numMiss)
                RESULT_RATE_SUM_ENTRY("@HitRate", numHit, numHit, numMiss)
            END_PARAM_MAP()

            Statistics();
            void SetName( const String& name );

            String name;
            u64 numHit;
            u64 numMiss;
        };

        std::vector<Statistics> m_statistics;
        Statistics m_totalStatistics;

        // Detect branch miss prediction and recovery if prediction is incorrect.
        void RecoveryFromBPredMiss( OpIterator branch );
    };

}; // namespace Onikiri

#endif // SIM_PREDICTOR_BPRED_BPRED_H


