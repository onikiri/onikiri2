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


#ifndef SIM_PIPELINE_DISPATCHER_DISPATCHER_H
#define SIM_PIPELINE_DISPATCHER_DISPATCHER_H

#include "Env/Param/ParamExchange.h"
#include "Sim/Pipeline/PipelineNodeBase.h"
#include "Sim/Op/OpContainer/OpExtraStateTable.h"
#include "Sim/Op/OpContainer/OpList.h"
#include "Sim/Foundation/Hook/HookDecl.h"
#include "Sim/Op/OpClassStatistics.h"
#include "Utility/Collection/fixed_size_buffer.h"

namespace Onikiri 
{

    class Scheduler;
    class DispatchSteererIF;

    class Dispatcher:
        public PipelineNodeBase
    {
        
    public:
        // An array of the numbers of dispatching ops.
        // Each array element corresponds to each scheduler.
        static const int MAX_SCHEDULER_NUM = 8;
        typedef 
            fixed_sized_buffer< int, MAX_SCHEDULER_NUM, Dispatcher >
            DispatchingOpNums;

        BEGIN_PARAM_MAP("")

            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY( "@DispatchLatency", m_dispatchLatency );
            END_PARAM_PATH()

            BEGIN_PARAM_PATH( GetResultPath() )
                BEGIN_PARAM_PATH( "NumStalledCycles/" )
                    RESULT_ENTRY( "@Total",         GetStalledCycles() )
                    RESULT_ENTRY( "@SchedulerFull", m_stallCylcles )
                END_PARAM_PATH()
                RESULT_ENTRY( "@SchedulerName", m_schedulerName )
                RESULT_ENTRY( "@NumDispatchedOps",  m_numDispatchedOps )
                
                // CHAIN_PARAM_MAP can treat 'std::vector<SchedulerInfo>'
                CHAIN_PARAM_MAP( "Scheduler", m_schedInfo ) 
                CHAIN_PARAM_MAP("OpStatistics", m_opClassStat)
            END_PARAM_PATH()

        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( Core,   "core",   m_core )
            RESOURCE_ENTRY( Thread, "thread", m_thread )
            RESOURCE_SETTER_ENTRY(
                PipelineNodeIF, "upperPipelineNode", SetUpperPipelineNode 
            )
        END_RESOURCE_MAP()

        Dispatcher();
        virtual ~Dispatcher();

        // PipelineNodeBase
        virtual void Initialize( InitPhase phase );
        virtual void Finalize();

        virtual void Evaluate();
        virtual void Update();
        virtual void ExitLowerPipeline( OpIterator op );
        
        virtual void Retire( OpIterator op );
        virtual void Flush( OpIterator op );

        // Accessors
        int GetDispatchLatency() const { return m_dispatchLatency; }

        //
        // --- Hook
        //

        struct DispatchHookParam
        {
            // The number of dispatching ops;
            const DispatchingOpNums* dispatchingOps;
            
            // It this flag is false, a corresponding op is not dispatched.
            bool dispatch;
        };

        // Prototype: void OnDispatchEvaluate( OpIterator, DispatchHookParam );
        // A hook point for evaluating a dispatched op.
        // Ops passed to an attached method are not dispatched
        // when a dispatcher is stalled.
        static HookPoint<Dispatcher, DispatchHookParam> s_dispatchEvaluateHook;

        // Prototype: void OnDispatchUpdate( OpIterator );
        // A hook point for updating an actually dispatched op.
        static HookPoint<Dispatcher, DispatchHookParam> s_dispatchUpdateHook;


    protected:

        typedef PipelineNodeBase BaseType;

        struct SchedulerInfo : public ParamExchangeChild
        {
            int index;
            s64 saturateCount;
            Scheduler* scheduler;
            OpList dispatchingOps;
            s64 numDispatchedOps;

            SchedulerInfo();

            BEGIN_PARAM_MAP( "" )
            END_PARAM_MAP()
        };

        int m_numScheduler;
        int m_dispatchLatency;

        std::vector< SchedulerInfo > m_schedInfo;
        std::vector< s64 > m_stallCylcles;
        std::vector< s64 > m_numDispatchedOps;
        std::vector< std::string > m_schedulerName;

        // Statistics of ops.
        OpClassStatistics m_opClassStat;

        SchedulerInfo* GetSchedulerInfo( OpIterator op );
        void Dispatch( OpIterator op );
        void Delete( OpIterator op, bool flush );
    };
    
}; // namespace Onikiri;

#endif // SIM_PIPELINE_DISPATCHER_DISPATCHER_H

