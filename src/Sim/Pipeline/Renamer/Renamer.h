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


#ifndef SIM_PIPELINE_RENAMER_RENAMER_H
#define SIM_PIPELINE_RENAMER_RENAMER_H

#include "Env/Param/ParamExchange.h"
#include "Sim/Pipeline/PipelineNodeBase.h"
#include "Sim/Foundation/Checkpoint/CheckpointMaster.h"
#include "Sim/Foundation/Hook/HookDecl.h"
#include "Sim/Op/OpClassStatistics.h"
#include "Utility/Collection/fixed_size_buffer.h"

namespace Onikiri 
{
    class RegDepPredIF;
    class MemDepPredIF;
    class CheckpointMaster;
    class MemOrderManager;
    class LatPred;
    class DispatchSteererIF;

    class Renamer:
        public PipelineNodeBase
    {
    public:

        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY( "@RenameLatency",  m_renameLatency );
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
                BEGIN_PARAM_PATH( "NumStalledCycles/" )
                    RESULT_ENTRY( "@Total",                 m_stallCycles.total )
                    RESULT_ENTRY( "@RegDepPredFull",        m_stallCycles.regDepPred )
                    RESULT_ENTRY( "@MemDepPredFull",        m_stallCycles.memDepPred )
                    RESULT_ENTRY( "@MemOrderManagerFull",   m_stallCycles.memOrderManager )
                    RESULT_ENTRY( "@Others",                m_stallCycles.others )
                END_PARAM_PATH()
                RESULT_ENTRY( "@NumRenamedOps", m_numRenamedOps )
                CHAIN_PARAM_MAP("OpStatistics", m_opClassStat)
            END_PARAM_PATH()
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( Core, "core", m_core )
            RESOURCE_ENTRY( Thread, "thread", m_thread )
            RESOURCE_ENTRY( RegDepPredIF,   "regDepPred",   m_regDepPred )
            RESOURCE_ENTRY( MemDepPredIF,   "memDepPred",   m_memDepPred )
            RESOURCE_ENTRY( CheckpointMaster, "checkpointMaster", m_checkpointMaster )
            RESOURCE_ENTRY( MemOrderManager,  "memOrderManager", m_memOrderManager )
            RESOURCE_ENTRY( LatPred,        "latPred",      m_latPred )
            RESOURCE_ENTRY( DispatchSteererIF, "steerer", m_steerer )
            RESOURCE_SETTER_ENTRY( PipelineNodeIF,  "upperPipelineNode", SetUpperPipelineNode )
        END_RESOURCE_MAP()

        Renamer();
        virtual ~Renamer();

        virtual void Initialize( InitPhase phase );
        virtual void Finalize();

        // --- PipelineNodeIF
        virtual void Update();
        virtual void Commit( OpIterator op );

        // --- ClockedResourceIF
        virtual void Evaluate();

        DispatchSteererIF* GetSteerer() const { return m_steerer; }

        // --- Hook points
        
        // Prototype: void OnRenameEvaluate( OpIterator );
        // A hook point for evaluating a renamed op.
        // Ops passed to an attached method do not be 
        // renamed when a renamer is stalled.
        static HookPoint< Renamer > s_renameEvaluateHook;   

        // Prototype: void OnRenameUpdate( OpIterator );
        // A hook point for updating a actually renamed op.
        static HookPoint< Renamer > s_renameUpdateHook;

        static HookPoint< Renamer > s_steerHook;

    protected:

        typedef PipelineNodeBase BaseType;

        // リネーム対象のOpを格納する配列
        static const int MAX_RENAMING_OPS = 16;
        typedef fixed_sized_buffer< OpIterator, MAX_RENAMING_OPS, Renamer > RenamingOpArray;

        // The latency of renaming
        int m_renameLatency;

        PhysicalResourceArray<RegDepPredIF>       m_regDepPred;     // レジスタの依存関係解析 
        PhysicalResourceArray<MemDepPredIF>       m_memDepPred;     // メモリの依存関係解析
        PhysicalResourceArray<CheckpointMaster>   m_checkpointMaster;
        PhysicalResourceArray<MemOrderManager>    m_memOrderManager;

        DispatchSteererIF* m_steerer;
        LatPred* m_latPred;

        struct StallCycles
        {
            s64 total;

            // Stalled by a register dependency predictor, which, it is typically a RMT. 
            // Typically, it is caused by the shortage of physical registers.
            s64 regDepPred;         
            // Stalled by a memory dependency predictor.
            s64 memDepPred;         
            // Stalled by th shortage of MemOrderManager(LSQ)
            s64 memOrderManager;    
            s64 others;

            StallCycles():
                total(0), regDepPred(0), memDepPred(0), memOrderManager(0), others(0)
            {
            }
        } m_stallCycles;
        s64 m_numRenamedOps;

        // Statistics of ops.
        OpClassStatistics m_opClassStat;

        // array内のopを引数として，メンバ関数ポインタから呼び出しを行う
        void ForEachOp( RenamingOpArray* c, void (Renamer::*func)(OpIterator) );
        
        template <typename T1>
        void ForEachOp1( RenamingOpArray* c, T1 arg1, void (Renamer::*func)(OpIterator, T1) );

        bool CanRename( RenamingOpArray* renamingOps );
        // リネームを行う関数
        void Rename( OpIterator );

        // NOPの処理を行う関数
        void ProcessNOP( OpIterator op );

        // Enter an op to a renmaer pipeline.
        // An op is send to a dispatcher automatically when the op exits a renamer pipeline.
        // A dispatcher is set to a renmaer by the dispatcher itself in SetUpperPipelineNode
        // that calls SetLowerPipelineNode of the renmaer.
        void EnterPipeline( OpIterator op );
        
        // Create an entry of MemOrderManager
        void CreateMemOrderManagerEntry( OpIterator op );

        // Process after renaming 
        // - Steer ops to schedulers.
        // - Predict latency of an op.
        //   Latency prediction is done on renaming stages.
        void Steer( OpIterator op );

        // リネームの前後で checkpoint にバックアップをとる関数
        void BackupOnCheckpoint( OpIterator op, bool before );
    };
    
}; // namespace Onikiri;

#endif  // SIM_PIPELINE_RENAMER_RENAMER_H


