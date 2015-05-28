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


#include <pch.h>

#include "Sim/Pipeline/Dispatcher/Dispatcher.h"

#include "Sim/Dumper/Dumper.h"
#include "Sim/Foundation/Hook/HookUtil.h"
#include "Sim/Op/Op.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/Core/Core.h"

using namespace std;
using namespace boost;
using namespace Onikiri;

namespace Onikiri 
{
    HookPoint<Dispatcher, Dispatcher::DispatchHookParam> Dispatcher::s_dispatchEvaluateHook;
    HookPoint<Dispatcher, Dispatcher::DispatchHookParam> Dispatcher::s_dispatchUpdateHook;
};  // namespace Onikiri

Dispatcher::SchedulerInfo::SchedulerInfo() :
    index(0),
    saturateCount(0),
    scheduler(0),
    numDispatchedOps(0)
{
}


Dispatcher::Dispatcher() :
    m_dispatchLatency(0)
{
}

Dispatcher::~Dispatcher()
{
}

void Dispatcher::Initialize( InitPhase phase )
{
    PipelineNodeBase::Initialize( phase );

    if( phase == INIT_PRE_CONNECTION ){
        LoadParam();
    }
    else if( phase == INIT_POST_CONNECTION ){

        // INIT_POST_CONNECTION
        m_numScheduler = GetCore()->GetNumScheduler();
        if( MAX_SCHEDULER_NUM < m_numScheduler ){
            THROW_RUNTIME_ERROR( 
                "The number of schedulers is greater than MAX_SCHEDULER_NUM." 
            );
        }

        ASSERT( m_schedInfo.size() == 0 );
        m_schedInfo.resize( m_numScheduler );

        for( int i = 0; i < m_numScheduler; ++i ){
            Scheduler* sched = GetCore()->GetScheduler(i);
            int index = sched->GetIndex();
            ASSERT( i == index, "Mismatched scheduler's indeces." );

            m_schedInfo[i].index = index;
            m_schedInfo[i].scheduler = sched;
            m_schedInfo[i].dispatchingOps.resize( *GetCore()->GetOpArray() );
        }
    
        // 新たにインスタンスが作成された（resizeにより）SchedulerSaturateCountを
        // ParamDBに登録するためにもう一度呼ぶ
        LoadParam();

        // member 変数のチェック
        if ( m_dispatchLatency == 0 ) {
            THROW_RUNTIME_ERROR( "A dispatch latency must be more than 0." );
        }
    }
}

void Dispatcher::Finalize()
{
    m_stallCylcles.clear();
    for( vector< SchedulerInfo >::iterator i = m_schedInfo.begin(); i != m_schedInfo.end(); ++i ){
        m_stallCylcles.push_back( i->saturateCount );
        m_schedulerName.push_back( i->scheduler->GetName() );
        m_numDispatchedOps.push_back( i->numDispatchedOps );
    }

    ReleaseParam();
}

Dispatcher::SchedulerInfo* Dispatcher::GetSchedulerInfo( OpIterator op )
{
    Scheduler* sched = op->GetScheduler();
    int index = sched->GetIndex();
    return &m_schedInfo[index];
}

void Dispatcher::Evaluate()
{
    BaseType::Evaluate();


    // Count the number of all dispatching ops.
    DispatchingOpNums dispatchingOps;
    for( int i = 0; i < m_numScheduler; i++ ){
        dispatchingOps.push_back( (int)m_schedInfo[i].dispatchingOps.size() );
    }

    DispatchHookParam hookParam;
    hookParam.dispatchingOps = &dispatchingOps;
    for( PipelineLatch::iterator i = m_latch.begin(); i != m_latch.end(); ++i ){
        // Count the number of dispatching ops.
        hookParam.dispatch = true;
        HOOK_SECTION_OP_PARAM( s_dispatchEvaluateHook, *i, hookParam ){
            if( hookParam.dispatch ){
                dispatchingOps[ (*i)->GetScheduler()->GetIndex() ]++;
            }
        }
    }

    // Can allocate enough entries in schedulers?
    bool stall = false;
    for( int i = 0; i < m_numScheduler; i++ ){
        if( !m_schedInfo[i].scheduler->CanAllocate( dispatchingOps[i] ) ){
            stall = true;
            m_schedInfo[i].saturateCount++;
        }
    }

    if( stall ){
        // Stall this node and upper pipelines.
        // Do not stall ops where dispatching is already 
        // started before this cycle.
        StallThisNodeAndUpperThisCycle();
    }
}

void Dispatcher::Dispatch( OpIterator op )
{   
    SchedulerInfo* schd = GetSchedulerInfo(op);
    op->SetStatus( OpStatus::OS_DISPATCHING );
    schd->dispatchingOps.push_back( op );
    //schd->pipeline->EnterPipeline( 
    GetLowerPipeline()->EnterPipeline(
        op, m_dispatchLatency - 1, op->GetScheduler() 
    );  
    schd->numDispatchedOps++;   
    m_opClassStat.Increment(op);
    g_dumper.Dump( DS_DISPATCH, op );
}

// Dispatch all ops in the latch if a dispatcher is not stalled.
void Dispatcher::Update()
{
    // Count the number of all dispatching ops.
    DispatchingOpNums dispatchingOps;
    for( int i = 0; i < m_numScheduler; i++ ){
        dispatchingOps.push_back( (int)m_schedInfo[i].dispatchingOps.size() );
    }
    DispatchHookParam hookParam;
    hookParam.dispatchingOps = &dispatchingOps;

    for( PipelineLatch::iterator i = m_latch.begin(); 
         i != m_latch.end(); 
         /*nothing*/ 
    ){
        OpIterator op = *i;
        hookParam.dispatch = true;
        HOOK_SECTION_OP_PARAM( s_dispatchUpdateHook, op, hookParam ){
            if( hookParam.dispatch ){
                Dispatch( op );
            }
        }
        i = m_latch.erase( i );
    }
}


void Dispatcher::ExitLowerPipeline( OpIterator op )
{
    SchedulerInfo* schd = GetSchedulerInfo( op );
    schd->dispatchingOps.find_and_erase( op );
}

void Dispatcher::Retire( OpIterator op )
{
    PipelineNodeBase::Retire( op );
    Delete( op, false );
}

void Dispatcher::Flush( OpIterator op )
{
    PipelineNodeBase::Flush( op );
    Delete( op, true );
}

void Dispatcher::Delete( OpIterator op, bool flush )
{

    if( op->GetStatus() < OpStatus::OS_DISPATCHING ||
        op->GetStatus() > OpStatus::OS_DISPATCHED
    ){
        return;
    }

    SchedulerInfo* schd = GetSchedulerInfo( op );
    schd->dispatchingOps.find_and_erase( op );
}
