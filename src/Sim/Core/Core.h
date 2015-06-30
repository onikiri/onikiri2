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


#ifndef SIM_CORE_CORE_H
#define SIM_CORE_CORE_H

#include "Utility/RuntimeError.h"
#include "Env/Param/ParamExchange.h"

#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/Foundation/Hook/HookDecl.h"
#include "Sim/Foundation/TimeWheel/ClockedResourceBase.h"

#include "Sim/Op/OpContainer/OpList.h"
#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Core/DataPredTypes.h"

namespace Onikiri 
{
    class EmulatorIF;
    class GlobalClock;
    class RegisterFile;
    class Fetcher;
    class Renamer;
    class Dispatcher;
    class Scheduler;
    class Retirer;
    class CheckpointMaster;
    class MemOrderManager;
    class OpNotifier;
    class ExecLatencyInfo;
    class ExecUnitIF;
    class LatPred;
    class Cache;
    class RegDepPredIF;
    class MemDepPredIF;
    class Thread;
    class LatPred;
    class CacheSystem; 
    class BPred;



    // コア
    class Core : 
        public ClockedResourceBase,
        public PhysicalResourceNode
    {
    public:

        // parameter mapping
        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY( "@OpArrayCapacity",    m_opArrayCapacity )
                
                BEGIN_PARAM_BINDING(  "@SchedulerRemovePolicy", m_schedulerRemovePolicy, SchedulerRemovePolicy )
                    PARAM_BINDING_ENTRY( "Remove",  RP_REMOVE )
                    PARAM_BINDING_ENTRY( "Retain",  RP_RETAIN )
                    PARAM_BINDING_ENTRY( "RemoveAfterFinish",   RP_REMOVE_AFTER_FINISH )
                END_PARAM_BINDING()

                BEGIN_PARAM_BINDING(  "@LoadPipelineModel", m_loadPipeLineModel, LoadPipelineModel )
                    PARAM_BINDING_ENTRY( "SingleIssue", LPM_SINGLE_ISSUE )
                    PARAM_BINDING_ENTRY( "MultiIssue",  LPM_MULTI_ISSUE )
                END_PARAM_BINDING()

                BEGIN_PARAM_BINDING(  "@CheckpointingPolicy", m_checkpointingPolicy, CheckpointingPolicy )
                    PARAM_BINDING_ENTRY( "All",     CP_ALL )
                    PARAM_BINDING_ENTRY( "Auto",    CP_AUTO )
                END_PARAM_BINDING()

                CHAIN_PARAM_MAP( "LatPredRecovery",       m_latencyPredRecv );
                CHAIN_PARAM_MAP( "AddrMatchPredRecovery", m_addrMatchPredRecv );
                CHAIN_PARAM_MAP( "ValuePredRecovery",     m_valuePredRecv );
                CHAIN_PARAM_MAP( "PartialLoadRecovery",   m_partialLoadRecovery );

            END_PARAM_PATH()
            BEGIN_PARAM_PATH( "/Session/Simulator/TimeWheelBase/" )
                PARAM_ENTRY("@Size", m_timeWheelSize);
            END_PARAM_PATH()
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( Thread,         "thread",       m_thread )
            RESOURCE_ENTRY( Fetcher,        "fetcher",      m_fetcher )
            RESOURCE_ENTRY( Renamer,        "renamer",      m_renamer )
            RESOURCE_ENTRY( Dispatcher,     "dispatcher",   m_dispatcher )
            RESOURCE_SETTER_ENTRY(
                            Scheduler,      "scheduler",    AddScheduler 
            )
            RESOURCE_ENTRY( Retirer,        "retirer",      m_retirer )
            RESOURCE_ENTRY( RegisterFile,   "registerFile", m_registerFile )
            RESOURCE_ENTRY( LatPred,        "latPred",      m_latPred )
            RESOURCE_ENTRY( ExecLatencyInfo,"execLatencyInfo", m_execLatencyInfo )
            RESOURCE_ENTRY( GlobalClock,    "globalClock",  m_globalClock )
            RESOURCE_ENTRY( CacheSystem,    "cacheSystem",  m_cacheSystem )
            RESOURCE_ENTRY( EmulatorIF,     "emulator",     m_emulator )
            RESOURCE_ENTRY( BPred,          "bPred",        m_bPred )
        END_RESOURCE_MAP()

        Core();
        virtual ~Core();

        // 初期化用メソッド
        void Initialize(InitPhase phase);

        // accessors
        void AddScheduler( Scheduler* scheduler );
        Scheduler* GetScheduler(int index);
        int GetNumScheduler() const;

        Thread* GetThread(int tid);
        int GetThreadCount();

        int GetCID(); 
        int GetTID(const int index);

        Fetcher*            GetFetcher()        const   { return m_fetcher;         }
        Renamer*            GetRenamer()        const   { return m_renamer;         }
        Dispatcher*         GetDispatcher()     const   { return m_dispatcher;      }
        Retirer*            GetRetirer()        const   { return m_retirer;         }
        RegisterFile*       GetRegisterFile()   const   { return m_registerFile;    }
        LatPred*            GetLatPred()         const  { return m_latPred;         }
        ExecLatencyInfo*    GetExecLatencyInfo() const  { return m_execLatencyInfo; }
        CacheSystem*        GetCacheSystem()    const   { return m_cacheSystem;     }
        BPred*              GetBPred()          const   { return m_bPred;           }

        GlobalClock* GetGlobalClock()   const { return m_globalClock;   }
        EmulatorIF*  GetEmulator()      const { return m_emulator;      }

        OpArray* GetOpArray() const     { return m_opArray; };

        const DataPredMissRecovery& GetLatPredMissRecovery()   const { return m_latencyPredRecv;   }
        const DataPredMissRecovery& GetAddrPredMissRecovery()  const { return m_addrMatchPredRecv;  }
        const DataPredMissRecovery& GetValuePredMissRecovery() const { return m_valuePredRecv; }
        const DataPredMissRecovery& GetPartialLoadRecovery()  const { return m_partialLoadRecovery;  }

        LoadPipelineModel       GetLoadPipelineModel()      const   {   return m_loadPipeLineModel; }
        SchedulerRemovePolicy   GetSchedulerRemovePolicy()  const   {   return m_schedulerRemovePolicy; }
        CheckpointingPolicy     GetCheckpointingPolicy()    const   {   return m_checkpointingPolicy; }

        int GetTimeWheelSize() const { return m_timeWheelSize; };

        // リカバリ方法
        // この命令がフェッチされる前のインオーダーなステートをチェックポインティングする必要がある
        // アクセスオーダーバイオレーションからの再フェッチによるリカバリに必要
        bool IsRequiredCheckpointBefore( const PC& pc, const OpInfo* const info );

            // この命令がフェッチされた後のインオーダーなステートをチェックポインティングする必要がある
        // 分岐予測ミスからのリカバリに必要
        bool IsRequiredCheckpointAfter( const PC& pc, const OpInfo* const info );

        // Cycle handler
        virtual void Evaluate();
        const char* Who() const 
        {
            return PhysicalResourceNode::Who(); 
        };

        //
        // --- Hook
        //
        struct CheckpointDecisionHookParam
        {   
            const PC* pc;
            const OpInfo* info;
            bool  before;
            bool  requried;
        };

        // The hook point of 'IsRequiredCheckpointBefore()' and 'IsRequiredCheckpointAfter()'
        // Prototype : void Method( HookParameter<Core, CheckpointDecisionHookParam>* param )
        static HookPoint<Core, CheckpointDecisionHookParam> s_checkpointDecisionHook;

    protected:

        // member variables
        int m_opArrayCapacity;      // OpArray の 最大サイズ
        int m_timeWheelSize;        // The size of a time wheel.

        GlobalClock* m_globalClock;
        OpArray*     m_opArray;

        typedef
            std::vector< Scheduler* >
            SchedulerArray;

        PhysicalResourceArray<Thread>    m_thread;
        RegisterFile*   m_registerFile;
        Fetcher*        m_fetcher;
        Renamer*        m_renamer;
        Dispatcher*     m_dispatcher;
        SchedulerArray  m_scheduler;
        Retirer*        m_retirer;
        LatPred*        m_latPred;
        CacheSystem*    m_cacheSystem;
        EmulatorIF*     m_emulator;
        BPred*          m_bPred;

        ExecLatencyInfo* m_execLatencyInfo; // 命令の実行レイテンシの情報

        DataPredMissRecovery m_latencyPredRecv;
        DataPredMissRecovery m_addrMatchPredRecv;
        DataPredMissRecovery m_valuePredRecv;
        DataPredMissRecovery m_partialLoadRecovery;

        LoadPipelineModel       m_loadPipeLineModel;
        SchedulerRemovePolicy   m_schedulerRemovePolicy;
        CheckpointingPolicy     m_checkpointingPolicy;
};

}; // namespace Onikiri

#endif // SIM_CORE_CORE_H

