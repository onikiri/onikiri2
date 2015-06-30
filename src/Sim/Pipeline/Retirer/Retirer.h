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


#ifndef SIM_PIPLINE_RETIRER_RETIRER_H
#define SIM_PIPLINE_RETIRER_RETIRER_H

#include "Types.h"
#include "Sim/Foundation/Hook/HookDecl.h"
#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Op/OpStatus.h"
#include "Sim/Pipeline/PipelineNodeBase.h"
#include "Sim/Op/OpClassStatistics.h"
#include "Utility/Collection/fixed_size_buffer.h"

namespace Onikiri
{
    class Thread;
    class ForwardEmulator;
    class InorderList;

    class Retirer : public PipelineNodeBase
    {
    public:

        // A hook parameter for s_commitSteeringHook.
        struct CommitSteeringHookParam
        {   
            PhysicalResourceArray<Thread>* threadList;
            Thread* targetThread;
        };

        // A hook parameter for s_commitDecisionHook.
        struct CommitDecisionHookParam
        {   
            OpIterator op;
            bool canCommit;
        };


        BEGIN_PARAM_MAP("")

            BEGIN_PARAM_PATH( GetParamPath() )

                PARAM_ENTRY( "@CommitWidth",    m_commitWidth )
                PARAM_ENTRY( "@RetireWidth",    m_retireWidth )
                PARAM_ENTRY( "@NoRetireLimit",  m_noCommitLimit )
                PARAM_ENTRY( "@CommitLatency",  m_commitLatency )
                PARAM_ENTRY( "@FixCommitLatency",   m_fixCommitLatency )

                BEGIN_PARAM_BINDING( "@CommittableStatus", m_committableStatus, OpStatus )
                    PARAM_BINDING_ENTRY( "WRITTENBACK", OpStatus::OS_WRITTEN_BACK )
                    PARAM_BINDING_ENTRY( "COMPLETED",   OpStatus::OS_COMPLETED )
                END_PARAM_BINDING()

            END_PARAM_PATH()

            BEGIN_PARAM_PATH( GetResultPath() )
                PARAM_ENTRY( "@NumCommittedOps",    m_numCommittedOps )
                PARAM_ENTRY( "@NumCommittedInsns",  m_numCommittedInsns )
                PARAM_ENTRY( "@NumRetiredOps",      m_numRetiredOps )
                PARAM_ENTRY( "@NumRetiredInsns",    m_numRetiredInsns )
                PARAM_ENTRY( "@NumStorePortFullStalledCycles",  m_numStorePortFullStalledCycles )
                CHAIN_PARAM_MAP("OpStatistics", m_opClassStat)
            END_PARAM_PATH()

        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( Thread, "thread",   m_thread )
            RESOURCE_ENTRY( Core,   "core",     m_core )
            RESOURCE_ENTRY( EmulatorIF,         "emulator",         m_emulator )
            RESOURCE_ENTRY( ForwardEmulator,    "forwardEmulator",  m_forwardEmulator )
        END_RESOURCE_MAP()


        Retirer();
        virtual ~Retirer();

        virtual void Initialize(InitPhase phase);

        // PipelineNodeIF
        virtual void Evaluate();
        virtual void Transition();
        virtual void Update();

        // accessors
        bool IsEndOfProgram()       const { return m_endOfProgram;      }
        s64 GetNumRetiredOp()       const { return m_numRetiredOps;     }
        s64 GetNumRetiredInsns()    const { return m_numRetiredInsns;   }

        // Set the number of retired ops/insns.
        // This is called when a simulation mode is changed from an emulation mode.
        void SetInitialNumRetiredOp( s64 numInsns, s64 numOp, s64 simulationEndInsns );

        // Called when the op is retired from OpRetireEvent.
        // Caution: this method is not called from InorderList because
        // the retirement of an op is triggered from this method.
        void Retire( OpIterator op );

        // Called when the op is flushed from InorderList.
        void Flush( OpIterator op );

        //
        // --- Hook
        //
        typedef HookPoint<Retirer> CommitHookPoint;
        static CommitHookPoint s_commitHook;
        
        // The hook point of 'GetComittableThread()'
        // Prototype : void Method( HookParameter<Retireer, SteeringHookParam>* param )
        typedef HookPoint<Retirer, CommitSteeringHookParam> CommitSteeringHookPoint;
        static CommitSteeringHookPoint s_commitSteeringHook;

        // The hook point of 'CanRetire()'
        // Prototype : void Method( HookParameter<Retireer,CommitDecisionHookPoint>* param )
        typedef HookPoint<Retirer, CommitDecisionHookParam> CommitDecisionHookPoint;
        static CommitDecisionHookPoint s_commitDecisionHook;

        typedef HookPoint<Retirer> RetireHookPoint;
        static RetireHookPoint s_retireHook;


    protected:
        typedef PipelineNodeBase BaseType;

        // Parameters
        int m_commitWidth;
        int m_retireWidth;
        int m_noCommitLimit;
        int m_commitLatency;
        bool m_fixCommitLatency;
        OpStatus m_committableStatus;

        // Statistics
        s64 m_numCommittedOps;
        s64 m_numCommittedInsns;
        s64 m_numRetiredOps;
        s64 m_numRetiredInsns;
        OpClassStatistics m_opClassStat; // Statistics of ops.
        
        // Stalled cycles when commitment is stalled for store ports full.
        s64 m_numStorePortFullStalledCycles;    

        int m_noCommittedCycle;
        s64 m_numSimulationEndInsns;

        // The index of a retireable thread in attached threads.
        int m_currentRetireThread;
        bool m_endOfProgram;

        EmulatorIF*         m_emulator;         // Emulator
        ForwardEmulator*    m_forwardEmulator;  // Forward Emulator.
        
        // Committed ops decided by Evaluate() in this cycle
        
        typedef pool_list< OpIterator > CommitingOps;
        struct Evaluated
        {
            CommitingOps    committing;

            bool            exceptionOccur;
            OpIterator      exceptionCauser;
            bool evaluated;
            bool storePortFull;

            void Reset();
        }
        m_evaluated;


        // --- Commit

        // Returns whether 'op' can commit or not.
        bool CanCommitOp( OpIterator op );

        // Returns whether 'ops' that belongs to the same instruction can commit or not.
        bool CanCommitInsn( OpIterator op );

        // Decide a thread that commits ops in this cycle.
        Thread* GetCommitableThread();

        void Commit( OpIterator op );
        void EvaluateCommit();
        void UpdateCommit();
        void UpdateException();

        // Update counters related to retirement.
        void CheckCommitCounters( int retiredOps, InorderList* inorderList );

        // Finish a thread.
        void FinishThread( Thread* tread );

    };

}; // namespace Onikiri

#endif // SIM_PIPLINE_RETIRER_RETIRER_H

