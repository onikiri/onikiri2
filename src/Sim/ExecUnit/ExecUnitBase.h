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


#ifndef SIM_EXEC_UNIT_EXEC_UNIT_BASE_H
#define SIM_EXEC_UNIT_EXEC_UNIT_BASE_H

#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/Foundation/Hook/HookDecl.h"
#include "Sim/ExecUnit/ExecUnitIF.h"
#include "Sim/ExecUnit/ExecUnitReserver.h"

namespace Onikiri 
{
    class ExecLatencyInfo;
    class Scheduler;
    class Core;
    struct IssueState;

    // 演算器に共通する実装
    class ExecUnitBase :
        public PhysicalResourceNode,
        public ExecUnitIF 
    {
    public:
        BEGIN_PARAM_MAP("")
            
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY( "@Name",   m_name )
                PARAM_ENTRY( "@Port",   m_numPorts )
                PARAM_ENTRY( "@Code",   m_codeStr )
            END_PARAM_PATH()
            
            BEGIN_PARAM_PATH( GetResultPath() )
                PARAM_ENTRY( "@Name",       m_name )
                PARAM_ENTRY( "@NumUsed",    m_numUsed )
                PARAM_ENTRY( "@NumUsable",  m_numUsable )
                RESULT_RATE_SUM_ENTRY("@UseRate", \
                    m_numUsed, m_numUsed, m_numUsable
                )
            END_PARAM_PATH()

        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( ExecLatencyInfo,    "execLatencyInfo",  m_execLatencyInfo )
            RESOURCE_ENTRY( Core,               "core",             m_core )
        END_RESOURCE_MAP()


        ExecUnitBase();
        virtual ~ExecUnitBase();
        virtual void Initialize( InitPhase phase );
        virtual void Finalize();

        virtual int GetMappedCode(int index);
        virtual int GetMappedCodeCount();

        // 実行レイテンシ後に FinishEvent を登録する
        virtual void Execute( OpIterator op );

        // OpCode から取りうるレイテンシの種類の数を返す
        virtual int GetLatencyCount( const OpClass& opClass );

        // OpCode とインデクスからレイテンシを返す
        virtual int GetLatency( const OpClass& opClass, int index );

        // 毎サイクル呼ばれる
        virtual void Begin();

        // Called in Update phase.
        virtual void Update();

        // accessors
        ExecLatencyInfo* GetExecLatencyInfo()
        {
            return m_execLatencyInfo;
        }

        virtual ExecUnitReserver* GetReserver()  
        {
            return &m_reserver;
        }

    protected:
        std::string m_name;
        int         m_numPorts;     // ExecUnitが発行可能なポート数

        // statistics
        u64 m_numUsed;
        u64 m_numUsable;

        // Latency の情報
        ExecLatencyInfo* m_execLatencyInfo;
        Core*            m_core;
        ExecUnitReserver m_reserver;

        void RegisterEvents( OpIterator op, const int latency );
        void RegisterFinishEvent( OpIterator op, const int latency );
        void RegisterRescheduleEvent( OpIterator op, const int latency, IssueState* issueState );
        void RegisterDetectEvent( OpIterator op, const int latency );

        std::vector<String> m_codeStr;
        std::vector<int>    m_code;
    };

}; // namespace Onikiri

#endif // SIM_EXEC_UNIT_EXEC_UNIT_BASE_H

