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

#include "Sim/System/SimulationSystem/SimulationSystem.h"

#include "Sim/Dumper/Dumper.h"
#include "Sim/Foundation/Hook/HookUtil.h"

#include "Sim/Foundation/TimeWheel/TimeWheel.h"
#include "Sim/Pipeline/Fetcher/Fetcher.h"
#include "Sim/Pipeline/Renamer/Renamer.h"
#include "Sim/Pipeline/Dispatcher/Dispatcher.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/Pipeline/Retirer/Retirer.h"
#include "Sim/InorderList/InorderList.h"


namespace Onikiri
{
    HookPoint<SimulationSystem> SimulationSystem::s_systemInitHook;

    HookPoint<SimulationSystem> SimulationSystem::s_cycleBeginHook;
    HookPoint<SimulationSystem> SimulationSystem::s_cycleEvaluateHook;
    HookPoint<SimulationSystem> SimulationSystem::s_cycleTransitionHook;
    HookPoint<SimulationSystem> SimulationSystem::s_cycleUpdateHook;
    HookPoint<SimulationSystem> SimulationSystem::s_cycleEndHook;
}

using namespace std;
using namespace boost;
using namespace Onikiri;

SimulationSystem::SimulationSystem()
{
    m_context = NULL;
}

void SimulationSystem::Run( SystemContext* context )
{
    try{
        m_context = context;
        InitializeResources();

        context->executedCycles = 1;
        s64 numInsns  = context->executionInsns;
        s64 numCycles = context->executionCycles;

        while(true){
            context->globalClock->Tick();

            for( int i = 0; i < context->threads.GetSize(); i++ ){
                Thread* thread = context->threads[i];
                g_dumper.SetCurrentInsnCount( thread, thread->GetCore()->GetRetirer()->GetNumRetiredInsns() );
                g_dumper.SetCurrentCycle( thread, context->executedCycles );
            }

            SimulateCycle();

            bool exitSimulation = true;
            s64 retiredInsns = 0;
            for( int i = 0; i < context->cores.GetSize(); i++ ){
                Core* core = context->cores[i];
                if( !core->GetRetirer()->IsEndOfProgram() ){
                    // Next PC が 0 になる分岐命令がリタイアしていたら終了
                    exitSimulation = false;
                }
                retiredInsns += core->GetRetirer()->GetNumRetiredInsns();
            }

            // 終了条件
            if(exitSimulation){
                break;
            }
            else if( numCycles > 0 ){
                // 実行サイクル数を指定した場合
                if( context->executedCycles >= numCycles ) 
                    break;
            }
            else{
                // 実行命令数を指定した場合
                if( retiredInsns >= numInsns )
                    break;
            }

            ++context->executedCycles;
        }

        // リタイアした命令数をupdate
        context->executedInsns.clear();
        for( int i = 0; i < context->threads.GetSize(); i++ ){
            context->executedInsns.push_back( context->threads[i]->GetInorderList()->GetRetiredInsns() );
        }
    }
    catch( std::runtime_error& error ){
        String msg;
        msg.format(
            "%s\n"
            "Last simulated cycle: %lld\n" ,
            error.what(),
            m_context->executedCycles
        );

        throw std::runtime_error( msg.c_str() );
    }
}

// Initialize all physical resources (method entry point)
void SimulationSystem::InitializeResources()
{
    s_systemInitHook.Trigger( this, this, HookType::HOOK_BEFORE );
    if( !s_systemInitHook.HasAround() ){
        InitializeResourcesBody();
        s_systemInitHook.Trigger( this, this, HookType::HOOK_AFTER );
    }
    else{
        s_systemInitHook.Trigger( this, this, HookType::HOOK_AROUND );
    }
}

// Initialize all physical resources (method body)
void SimulationSystem::InitializeResourcesBody()
{
    PhysicalResourceArray<Core>&   core   = m_context->cores;
    PhysicalResourceArray<Thread>& thread = m_context->threads;
    PhysicalResourceArray<Cache>&  caches = m_context->caches;

    m_coreResources.resize( core.GetSize() );
    m_threadResources.resize( thread.GetSize() );
    m_memResources.resize( caches.GetSize() );

    for( int i = 0; i < thread.GetSize(); i++ ){
        ThreadResources& res = m_threadResources[i];
        res.memOrderManager = thread[i]->GetMemOrderManager();
    }

    //
    // Register clocked resources
    //
    for( int i = 0; i < caches.GetSize(); i++ ){
        m_clockedResources.push_back( caches[i] );
    }

    for( int i = 0; i < core.GetSize(); i++ ){
        CoreResources& res = m_coreResources[i];

        res.core = core[i];
        m_clockedResources.push_back( core[i]->GetRetirer() );

        for( int j = 0; j < core[i]->GetNumScheduler(); j++ ){
            Scheduler* scheduler = core[i]->GetScheduler( j );
            m_clockedResources.push_back( scheduler );
        }

        m_clockedResources.push_back( core[i]->GetDispatcher() );
        m_clockedResources.push_back( core[i]->GetRenamer() );
        m_clockedResources.push_back( core[i]->GetFetcher() );

        m_clockedResources.push_back( core[i] );
    }

    sort(
        m_clockedResources.begin(), 
        m_clockedResources.end(), 
        ClockedResourceBase::ComparePriority() 
    );

    //
    // Register time wheels 
    //
    for( int i = 0; i < caches.GetSize(); i++ ){
        m_timeWheels.push_back( caches[i]->GetLowerPipeline() );
    }

    for( int i = 0; i < core.GetSize(); i++ ){
        CoreResources& res = m_coreResources[i];

        res.core = core[i];
        m_timeWheels.push_back( core[i]->GetRetirer()->GetLowerPipeline() );

        for( int j = 0; j < core[i]->GetNumScheduler(); j++ ){
            Scheduler* scheduler = core[i]->GetScheduler( j );
            m_timeWheels.push_back( scheduler->GetLowerPipeline() );
        }

        m_timeWheels.push_back( core[i]->GetDispatcher()->GetLowerPipeline() );
        m_timeWheels.push_back( core[i]->GetRenamer()->GetLowerPipeline() );
        m_timeWheels.push_back( core[i]->GetFetcher()->GetLowerPipeline() );
    }


}

SimulationSystem::SystemContext* SimulationSystem::GetContext()
{
    return m_context;
}

void SimulationSystem::CycleBegin()
{
    ClockedResourceList::iterator end = m_clockedResources.end();
    for( ClockedResourceList::iterator i = m_clockedResources.begin(); i != end; ++i ){
        (*i)->Begin();
    }
}

void SimulationSystem::CycleEvaluate()
{
    m_priorityEventList.ExtractEvent( &m_timeWheels );

    m_priorityEventList.BeginEvaluate();

    ClockedResourceList::iterator end = m_clockedResources.end();
    for( ClockedResourceList::iterator i = m_clockedResources.begin(); i != end; ++i ){
        ClockedResourceIF* res = *i;
        m_priorityEventList.TriggerEvaluate( res->GetPriority() );
        res->Evaluate();
    }

    m_priorityEventList.EndEvaluate();
}

void SimulationSystem::CycleTransition()
{
    ClockedResourceList::iterator end = m_clockedResources.end();
    for( ClockedResourceList::iterator i = m_clockedResources.begin(); i != end; ++i ){
        (*i)->Transition();
    }
}

void SimulationSystem::CycleUpdate()
{   
    m_priorityEventList.BeginUpdate();

    ClockedResourceList::iterator end = m_clockedResources.end();
    for( ClockedResourceList::iterator i = m_clockedResources.begin(); i != end; ++i ){
        ClockedResourceIF* res = *i;
        m_priorityEventList.TriggerUpdate( res->GetPriority() );
        res->TriggerUpdate();
    }

    m_priorityEventList.EndUpdate();
}

void SimulationSystem::CycleEnd()
{
    ClockedResourceList::iterator end = m_clockedResources.end();
    for( ClockedResourceList::iterator i = m_clockedResources.begin(); i != end; ++i ){
        (*i)->End();
    }
}

void SimulationSystem::SimulateCycle()
{
    if( s_cycleBeginHook.IsAnyHookRegistered()   || 
        s_cycleEvaluateHook.IsAnyHookRegistered() ||
        s_cycleTransitionHook.IsAnyHookRegistered() ||
        s_cycleUpdateHook.IsAnyHookRegistered() ||
        s_cycleEndHook.IsAnyHookRegistered()
    ){
        HookEntry( this, &SimulationSystem::CycleBegin,      &s_cycleBeginHook );
        HookEntry( this, &SimulationSystem::CycleEvaluate,   &s_cycleEvaluateHook );
        HookEntry( this, &SimulationSystem::CycleTransition, &s_cycleTransitionHook );
        HookEntry( this, &SimulationSystem::CycleUpdate,     &s_cycleUpdateHook );
        HookEntry( this, &SimulationSystem::CycleEnd,        &s_cycleEndHook );
    }
    else{
        CycleBegin();
        CycleEvaluate();
        CycleTransition();
        CycleUpdate();
        CycleEnd();
    }

}
