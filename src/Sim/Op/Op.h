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


#ifndef SIM_OP_OP_H
#define SIM_OP_OP_H

#include "Interface/Addr.h"
#include "Interface/MemIF.h"
#include "Interface/OpStateIF.h"
#include "Interface/OpInfo.h"

#include "Sim/ISAInfo.h"

#include "Sim/Foundation/Hook/HookDecl.h"
#include "Sim/Foundation/Event/EventList/EventList.h"
#include "Sim/Foundation/Debug.h"
#include "Sim/Foundation/Resource/ResourceBase.h"

#include "Sim/Dependency/MemDependency/MemDependency.h"
#include "Sim/Predictor/LatPred/LatPredResult.h"
#include "Sim/Pipeline/Scheduler/IssueState.h"
#include "Sim/Op/OpStatus.h"
#include "Sim/Memory/Cache/CacheTypes.h"
#include "Sim/Recoverer/Exception.h"

namespace Onikiri
{
    class InorderList;
    class Core;
    class Thread;
    class Scheduler;
    class Checkpoint;
    class TimeWheelBase;
    class RegisterFile;
    class PhyReg;
    class ExecUnitIF;
    struct OpInitArgs;

    // 命令をあらわすクラス
    class Op :
        public OpStateIF,
        public LogicalResourceBase
    {
    public:
        static const int UNSET_REG = -1;
        static const int MAX_SRC_MEM_NUM = 1;
        static const int MAX_DST_MEM_NUM = 1;

        typedef EventList::MaskType EventMask;
        static const int EVENT_MASK_DEFAULT        = 1 << EventList::EVENT_MASK_POS_DEFAULT;
        static const int EVENT_MASK_WAKEUP_RELATED = 1 << EventList::EVENT_MASK_POS_USER;
        static const int EVENT_MASK_ALL            = EventList::EVENT_MASK_ALL;


        Op(OpIterator iterator);
        virtual ~Op();

        // Initialize
        void Initialize(const OpInitArgs& args);

        // OpStateIF
        virtual PC GetPC() const { return m_pc; }
        virtual const u64 GetSrc(const int index) const;
        virtual void SetDst(const int index, const u64 value);
        virtual const u64 GetDst(const int index) const;
        virtual void SetTakenPC(const PC takenPC);
        virtual PC GetTakenPC() const;
        virtual void SetTaken(const bool taken);
        virtual bool GetTaken() const;

        // MemIF
        virtual void Read(  MemAccess* access );
        virtual void Write( MemAccess* access );

        // Get a pc of a next of.
        PC GetNextPC();

        // Returns whether this op is ready to select in an 'index'-th scheduler.
        // 'newDeps' is a set of dependencies that will be satisfied.
        // You can test whether an op can be selected if 'newDeps' dependencies are 
        // satisfied. 'newDeps' argument can be NULL.
        bool IsSrcReady( int index, const DependencySet* newDeps = NULL ) const;
        
        // イベントをOp 自身とTimeWheel に登録
        void AddEvent(
            const EventPtr& evnt, 
            TimeWheelBase* timeWheel, 
            int time, 
            EventMask mask = EVENT_MASK_DEFAULT 
        );
        // Op に登録されているイベントをキャンセル
        void CancelEvent( 
            EventMask mask = EVENT_MASK_ALL
        );

        // Op に登録されているイベントをクリア
        void ClearEvent();
        void ClearWakeupEvent();

        // Re-schedule a self.
        // This method clears the events and the dependencies.
        // This method must be called from Scheduler::Reschedule().
        // If you need to re-schedule an op, use Scheduler::Reschedule().
        void RescheduleSelf( bool clearIssueState );        

        // 実行の処理
        void ExecutionBegin();  // Emulation is executed.
        void ExecutionEnd();    // Emulation result is written back and update a state.

        // Write execution results to physical registers.
        void WriteExecutionResults();   

        // dependency まわり
        void DissolveSrcReg();
        void DissolveSrcMem();

        void SetSrcReg(int index, int phyRegNo);
        int GetSrcReg(int index);

        void SetDstReg(int index, int phyRegNo);
        int GetDstReg(int index);

        int GetDstRegNum() const { return m_dstNum ;}
        int GetSrcRegNum() const { return m_srcNum ;}

        PhyReg* GetDstPhyReg(int index) const { return m_dstPhyReg[index]; }

        void SetSrcMem(int index, MemDependencyPtr memDep);
        MemDependencyPtr GetSrcMem(int index) const { return m_srcMem[index]; }

        void SetDstMem(int index, MemDependencyPtr dstMem);
        MemDependencyPtr GetDstMem(int index) const { return m_dstMem[index]; }

        int GetDstDepNum() const;
        Dependency* GetDstDep( int index ) const;

        OpIterator GetFirstConsumer();
        void ResetDependency();

        // Op がDispatche 以降のステージにあるかどうか
        bool IsDispatched() const
        {
            OpStatus status = GetStatus();
            return ( status >= OpStatus::OS_DISPATCHED && status != OpStatus::OS_NOP );
        }

        //
        // accessors
        //
        int GetLocalTID()           const { return m_localTID;             }
        OpInfo* const GetOpInfo()   const { return m_opInfo;               }
        const OpClass& GetOpClass() const { return *m_opClass;             }

        int GetNo()             const { return m_no; }
        u64 GetSerialID()       const { return m_serialID; }                // 各スレッド内でのSerialID
        u64 GetRetireID()       const { return m_retireID; }                
        u64 GetGlobalSerialID() const { return m_globalSerialID; }          // コア内全スレッドにおけるSerialID

        Thread*         GetThread()         const { return m_thread;      }
        Core*           GetCore()           const { return m_core;        }
        InorderList*    GetInorderList()    const { return m_inorderList; }

        void SetStatus( OpStatus status )   { m_status = status; }
        OpStatus GetStatus() const          { return m_status;   }

        void SetMemAccess( const MemAccess& memAccess ) { m_memAccess = memAccess; }
        const MemAccess& GetMemAccess() const           { return m_memAccess;      }

        void SetException( const Exception& exception) { m_exception = exception; }
        const Exception& GetException() const          { return m_exception;      }

        void SetCacheAccessResult( const CacheAccessResult& cacheAccessResult )
        { m_cacheAccessResult = cacheAccessResult;  }
        const CacheAccessResult& GetCacheAccessResult() const             
        { return m_cacheAccessResult;               }

        void SetPredPC(const PC predPC) { m_predPC = predPC; }
        PC GetPredPC() const            { return m_predPC; }

        void SetBeforeCheckpoint(Checkpoint* checkpoint) { m_beforeCheckpoint = checkpoint; }
        Checkpoint* GetBeforeCheckpoint() const          { return m_beforeCheckpoint; }
        void SetAfterCheckpoint(Checkpoint* checkpoint)  { m_afterCheckpoint = checkpoint; }
        Checkpoint* GetAfterCheckpoint() const           { return m_afterCheckpoint; }

        void SetLatPredRsult( const LatPredResult& result ) { m_latPredResult = result; }
        const LatPredResult& GetLatPredRsult() const        { return m_latPredResult;   }
        
        void SetIssueState( const IssueState& issueState )  { m_issueState = issueState; }
        const IssueState& GetIssueState() const             { return m_issueState;       }

        void SetReserveExecUnit( bool reserveExecUnit ) { m_reserveExecUnit = reserveExecUnit;  }
        bool GetReserveExecUnit() const                 { return m_reserveExecUnit;             }

        void SetScheduler(Scheduler* scheduler) { m_scheduler = scheduler; }
        Scheduler*  GetScheduler()  const       { return m_scheduler; }
        ExecUnitIF* GetExecUnit()   const;

        const OpIterator GetIterator() const       { return m_iterator; }

        //
        // methods for debugging
        //
        std::string ToString(int detail = 5, bool valDetail = false, const char* = "\t");

    protected:

        // PC
        PC m_pc;
        // PCに対応する命令の情報
        OpInfo*  m_opInfo;
        const OpClass* m_opClass;

        // 同一PC内に複数のOpがあるときに、いくつ目かを特定する番号
        // 上流側から順に 0 1 2 ...
        int m_no;

        // Local thread id in each core
        int m_localTID;

        // フェッチされた順につく番号
        u64 m_serialID;
        u64 m_globalSerialID;
        // リタイアした順につく番号
        u64 m_retireID;

        // フェッチされたコア/スレッド/InorderList
        Thread*     m_thread;
        Core*           m_core;
        InorderList*    m_inorderList;

        // 現在どういう状態なのか
        OpStatus m_status;

        // 自分が作ったチェックポイント
        Checkpoint* m_beforeCheckpoint;
        Checkpoint* m_afterCheckpoint;

        // 分岐が taken だった時のPC
        PC m_takenPC;
        // 分岐が taken かどうか
        bool m_taken;

        // Memory access
        MemAccess   m_memAccess;

        // Cache access result
        CacheAccessResult m_cacheAccessResult;

        // Exception
        Exception m_exception;

        // 予測したPC
        PC m_predPC;

        // デスティネーションに割り当てられた物理レジスタの番号
        int m_dstReg[SimISAInfo::MAX_DST_REG_COUNT];
        // ソースに割り当てられた物理レジスタの番号
        int m_srcReg[SimISAInfo::MAX_SRC_REG_COUNT];

        // デスティネーションに割り当てられたメモリの依存関係のポインタ
        MemDependencyPtr m_dstMem[MAX_DST_MEM_NUM];
        // ソースに割り当てられたメモリの依存関係のポインタ
        MemDependencyPtr m_srcMem[MAX_SRC_MEM_NUM];

        // Result of latency prediction
        LatPredResult m_latPredResult;

        // State of issuing
        IssueState m_issueState;

        // Whether reserving an execution unit or not.
        bool m_reserveExecUnit;

        // フラッシュされるときに消すイベント
        EventList m_event;

        // 自分が今いるスケジューラ
        Scheduler* m_scheduler;

        // レジスタファイル
        RegisterFile* m_regFile;

        // ソースとディスティネーション
        int m_srcNum;
        int m_dstNum;
        PhyReg* m_dstPhyReg[SimISAInfo::MAX_DST_REG_COUNT];
        PhyReg* m_srcPhyReg[SimISAInfo::MAX_SRC_REG_COUNT];
        u64     m_dstResultValue[SimISAInfo::MAX_DST_REG_COUNT];

        OpIterator m_iterator;

        PhyReg* GetPhyReg(int phyRegNo);
    };

    class OpHash
    {
    public:
        size_t operator()(const Op* op) const
        {
            static const size_t rdiv = 0x100000 / sizeof(Op); 
            return (size_t)op * rdiv / 0x100000;
        }
    };

}; // namespace Onikiri

#endif // SIM_OP_OP_H

