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


// Scheduler
// This class schedules ops on processor back-end stages.
// (from wakeup/select to write back).


#ifndef SIM_PIPELINE_SCHEDULER_SCHEDULER_H
#define SIM_PIPELINE_SCHEDULER_SCHEDULER_H

#include "Env/Param/ParamExchange.h"

#include "Sim/Pipeline/PipelineNodeBase.h"
#include "Sim/Pipeline/Pipeline.h"
#include "Sim/Foundation/Hook/HookUtil.h"

#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Op/OpContainer/OpExtraStateTable.h"
#include "Sim/Op/OpContainer/OpBuffer.h"
#include "Sim/Op/OpClassStatistics.h"
#include "Sim/Core/DataPredTypes.h"
#include "Sim/Dependency/Dependency.h"

namespace Onikiri 
{
    
    struct IssueState;
    class Thread;
    class LatPred;
    class ExecUnitIF;
    class IssueSelectorIF;

    class Scheduler :
        public PipelineNodeBase
    {
    public:

        static const int MAX_SCHEDULING_OPS = 4096;
        typedef fixed_sized_buffer< OpIterator, MAX_SCHEDULING_OPS, Scheduler > SchedulingOps;
        //typedef pool_vector< OpIterator > SchedulingOps;


        // parameter mapping
        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH(GetParamPath())
                PARAM_ENTRY("@Index",           m_index);
                PARAM_ENTRY("@IssueWidth",      m_issueWidth);
                PARAM_ENTRY("@IssueLatency" ,   m_issueLatency);
                PARAM_ENTRY("@WritebackLatency" ,m_writeBackLatency);
                PARAM_ENTRY("@CommunicationLatency",    m_communicationLatency);
                PARAM_ENTRY("@WindowCapacity",  m_windowCapacity);
                BEGIN_PARAM_BINDING(  "@RemovePolicy", m_removePolicyParam, SchedulerRemovePolicy )
                    PARAM_BINDING_ENTRY( "Core",    RP_FOLLOW_CORE )
                    PARAM_BINDING_ENTRY( "Remove",  RP_REMOVE )
                    PARAM_BINDING_ENTRY( "Retain",  RP_RETAIN )
                    PARAM_BINDING_ENTRY( "RemoveAfterFinish",   RP_REMOVE_AFTER_FINISH )
                END_PARAM_BINDING()
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
                CHAIN_PARAM_MAP("IssuedOpStatistics", m_issuedOpClassStat)
            END_PARAM_PATH()
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( Core, "core", m_core )
            RESOURCE_ENTRY( Thread, "thread", m_thread )
            RESOURCE_SETTER_ENTRY( ExecUnitIF, "execUnits", SetExecUnit )
            RESOURCE_ENTRY( IssueSelectorIF, "selector", m_selector )
        END_RESOURCE_MAP()

        Scheduler();
        virtual ~Scheduler();

        void Initialize(InitPhase phase);

        // code に対応する演算器をセット
        void SetExecUnit( PhysicalResourceArray<ExecUnitIF>& execUnits );

        // code に対応する演算器を得る
        ExecUnitIF* GetExecUnit(int code);

        // PipelineNodeIF
        virtual void Begin();
        virtual void Evaluate();
        virtual void Transition();
        virtual void Update();

        virtual void Commit( OpIterator op );
        virtual void Cancel( OpIterator op );
        virtual void Flush( OpIterator op );
        virtual void Retire( OpIterator op );
        virtual void ExitUpperPipeline( OpIterator op );

        // Set a dependency satisfied in this cycle.
        // This method is called from WakeupEvent::Evaluate().
        void EvaluateDependency( OpIterator op );

        // op に依存する命令をwake up するイベントを登録する
        void RegisterWakeUpEvent( OpIterator op, int latencyFromOp );   

        // Returns whether 'op' can be selected in this cycle.
        // This method must be called only in a 'Evaluate' phase.
        bool CanSelect( OpIterator op );

        // Reserves 'op' to select in this cycle.
        // This method must be called only in a 'Evaluate' phase.
        void ReserveSelect( OpIterator op );

        // issue
        void Issue( OpIterator op );

        // Finish
        void Finished( OpIterator op );

        // Write Back
        void WriteBackBegin( OpIterator op );
        void WriteBackEnd( OpIterator op );


        // Re-schedule an op.
        // An op is returned to not issued state (DISPATCHED) and 
        // re-dcheduled.
        bool Reschedule( OpIterator op );

        // Return whether a scheduler can dispatch 'ops' or not.
        bool CanAllocate( int ops );

        // Check whether an op is in this scheduler.
        bool IsInScheduler( OpIterator op );

        // Returns the number of ops in this scheduler, which corresponds to an
        // issue queue.
        int GetOpCount();

        // accessors
        int GetIndex() const            { return m_index;  }
        void SetIndex(int index)        { m_index = index; }
        const std::vector<ExecUnitIF*>& 
            GetExecUnitList() const     { return m_execUnit; }
        int GetIssueLatency() const     { return m_issueLatency; }
        int GetIssueWidth()   const     { return m_issueWidth; }

        const OpList& GetReadyOps()         const   {   return m_readyOp;       }
        const OpList& GetNotReadyOps()      const   {   return m_notReadyOp;    }
        const OpBuffer& GetIssuedOps()      const   {   return m_issuedOp;      }
        const SchedulingOps& GetWokeUpOps() const   {   return m_evaluated.wokeUp;      } // Woke up ops in this cycle.


        // Hooks
        struct RescheduleHookParam
        {
            bool canceled;      // If op is canceled, this parameter is set to true.
        };

        static HookPoint<Scheduler> s_dispatchedHook;
        static HookPoint<Scheduler> s_readySigHook;
        static HookPoint<Scheduler> s_wakeUpHook;
        static HookPoint<Scheduler> s_selectHook;
        static HookPoint<Scheduler> s_issueHook;
        static HookPoint<Scheduler, RescheduleHookParam> s_rescheduleHook;

    private:
        typedef PipelineNodeBase BaseType;

        OpList      m_notReadyOp;       // まだreadyになっていないop
        OpList      m_readyOp;          // readyになってselectの対象になるop
        OpBuffer    m_issuedOp;         // issueされたop

        struct Evaluated
        {
            DependencySet   deps;       // Dependencies satisfied in this cycle.
                                        // Waking-up is done with these dependencies.
            SchedulingOps   wokeUp; // Woke up ios in this cycle.
            SchedulingOps   selected;   // Selected ops in this cycle.
        } m_evaluated;

        int m_index;                // 何番目のスケジューラか
        int m_issueWidth;           // 発行幅
        int m_issueLatency;         // 発行レイテンシ
        int m_writeBackLatency;     // The latency of write back.
        std::vector<int> 
            m_communicationLatency; // スケジューラ間の通信レイテンシ
        int m_windowCapacity;       // 命令ウインドウのサイズ

        // Load pipeline model.
        // See comments in 'Core.h'
        LoadPipelineModel m_loadPipelineModel;

        // Whether forget or retain ops in a scheduler after issue.
        SchedulerRemovePolicy m_removePolicyParam;
        SchedulerRemovePolicy m_removePolicy;

        // Execution units.
        std::vector<ExecUnitIF*> m_execUnit;
        std::vector<ExecUnitIF*> m_execUnitCodeMap;

        // Selector
        IssueSelectorIF* m_selector;

        // スケジューラのクラスタ
        struct Cluster
        {
            Scheduler*   scheduler;
            int          issueLatency;          // 発行レイテンシ
            int          communicationLatency;  // スケジューラ間の通信レイテンシ
        };
        std::vector<Cluster> m_clusters;

        // Statistics of ops.
        OpClassStatistics m_issuedOpClassStat;

        // Clear evaluated conctext.
        void ClearEvaluated();

        // ディスパッチされてきたopを受け取る
        void DispatchEnd(OpIterator op);

        // wake up
        void EvaluateWakeUp();
        void UpdateWakeUp();
        void WakeUp(OpIterator op);
        void CheckOnWakeUp(OpIterator op);

        // select
        void EvaluateSelect();
        void UpdateSelect();
        void Select(OpIterator op);
        void SelectBody(OpIterator op);

        // issue
        void IssueBody(OpIterator op);

        // Retire/Flush
        void Delete( OpIterator op );

        // opを実行するイベントを登録する
        void RegisterExecuteEvent(OpIterator op, int latency);

    };
    
}; // namespace Onikiri;

#endif

