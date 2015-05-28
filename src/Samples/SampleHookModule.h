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


//
// This is a sample class to demonstrate how to use hook mechanisms.
// You can activate this class by defining USE_SAMPLE_HOOK_MODULE.
//

#ifndef SAMPLES_SAMPLE_HOOK_MODULE_H
#define SAMPLES_SAMPLE_HOOK_MODULE_H

// USE_SAMPLE_HOOK_MODULE is referred in 'User/UserResourceMap.h' and 'User/UserInit.h'.
//#define USE_SAMPLE_HOOK_MODULE 1


#include "Sim/Foundation/Resource/ResourceNode.h"

#include "Sim/Pipeline/Fetcher/Fetcher.h"
#include "Sim/Pipeline/Renamer/Renamer.h"
#include "Sim/Pipeline/Dispatcher/Dispatcher.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/Pipeline/Scheduler/FinishEvent.h"
#include "Sim/Pipeline/Retirer/Retirer.h"
#include "Sim/InorderList/InorderList.h"
#include "Sim/System/SimulationSystem/SimulationSystem.h"

#include "Sim/Op/Op.h"

// All Onikiri classes are placed in a 'Onikiri' namespace.
namespace Onikiri
{
    // All modules must inherit PhysicalResourceNode.
    class SampleHookModule : public PhysicalResourceNode
    {
    public:

        void OnOpFetch( Fetcher::FetchHookParam* param )
        {
            if( param->op.IsNull() ){
                g_env.Print( "op is not fetched yet.\n" );
            }
            else{
                g_env.Print( "op(%s) is fetched.\n", param->op->ToString(0).c_str() );
            }
        }

        void OnOpRename( HookParameter<Renamer>* param )
        {
            g_env.Print( "op(%s) is renamed.\n", param->GetOp()->ToString(0).c_str() );
        }

        void OnOpDispatch( HookParameter<Dispatcher, Dispatcher::DispatchHookParam>* param )
        {
            g_env.Print( "op(%s) is dispatched.\n", param->GetOp()->ToString(0).c_str() );
        }

        void OnOpIssue( HookParameter<Scheduler>* param )
        {
            g_env.Print( "op(%s) is issued.\n", param->GetOp()->ToString(0).c_str() );
        }

        void OnOpExecutionFinish( OpIterator op, OpFinishEvent::FinishHookParam* param )
        {
            if( op.IsAlive() ){
                g_env.Print( "op(%s) is executed.\n", op->ToString(0).c_str() );
            }
            else{
                g_env.Print( "op is flushed.\n" );
            }
        }

        void OnOpRetire( HookParameter<Retirer>* param )
        {
            g_env.Print( "op(%s) is retired.\n", param->GetOp()->ToString(0).c_str() );
        }

        void OnOpFlushed( HookParameter<InorderList>* param )
        {
            g_env.Print( "op(%s) is flushed.\n", param->GetOp()->ToString(0).c_str() );
        }

        void OnOpRescheduled( OpIterator op, Scheduler::RescheduleHookParam* param )
        {
            g_env.Print( "op(%s) is re-scheduled.\n", op->ToString(0).c_str() );
        }

        void OnCycleBegin()
        {
            g_env.Print( "--- Cycle Begin\n" );
        }

        void OnCycleEvaluate()
        {
            g_env.Print( "On a cycle evaluation phase.\n" );
        }

        void OnCycleProcess()
        {
            g_env.Print( "On a cycle process phase.\n" );
        }

        SampleHookModule(){};
        virtual ~SampleHookModule(){};

        // Initialize and Finalize must be implemented.
        virtual void Initialize( InitPhase phase )
        {
            if( phase == INIT_PRE_CONNECTION ){
                // After constructing and before object connection.
                // LoadParam() must be called in this phase or later.
                LoadParam();
            }
            else if ( phase == INIT_POST_CONNECTION ){
                // After connection

                // Register the handlers to the hooks.

                // To pipeline stages.
                Fetcher::s_fetchHook.Register( 
                    this, &SampleHookModule::OnOpFetch, 0, HookType::HOOK_AFTER 
                );
                Renamer::s_renameUpdateHook.Register( 
                    this, &SampleHookModule::OnOpRename, 0, HookType::HOOK_AFTER 
                );
                Dispatcher::s_dispatchUpdateHook.Register( 
                    this, &SampleHookModule::OnOpDispatch, 0, HookType::HOOK_AFTER 
                );
                Scheduler::s_issueHook.Register(
                    this, &SampleHookModule::OnOpIssue, 0, HookType::HOOK_AFTER
                );
                OpFinishEvent::s_finishHook.Register(
                    this, &SampleHookModule::OnOpExecutionFinish, 0, HookType::HOOK_AFTER 
                );
                
                // A hook handler for retirement needs to be set with HOOK_BEFORE,
                // because 'op' cannot be used after retirement (HOOK_AFTER).
                Retirer::s_retireHook.Register(
                    this, &SampleHookModule::OnOpRetire, 0, HookType::HOOK_BEFORE
                );


                // Flush/Re-schedule
                // 'OnOpFlushed()' needs to be set with HOOK_BEFORE for the similar
                // reason in retirement.
                InorderList::s_opFlushHook.Register(
                    this, &SampleHookModule::OnOpFlushed, 0, HookType::HOOK_BEFORE
                );
                Scheduler::s_rescheduleHook.Register(
                    this, &SampleHookModule::OnOpRescheduled, 0, HookType::HOOK_AFTER
                );


                // Cycle hook handlers
                SimulationSystem::s_cycleBeginHook.Register(
                    this, &SampleHookModule::OnCycleBegin, 0, HookType::HOOK_AFTER
                );
                SimulationSystem::s_cycleEvaluateHook.Register(
                    this, &SampleHookModule::OnCycleEvaluate, 0, HookType::HOOK_AFTER
                );
                SimulationSystem::s_cycleUpdateHook.Register(
                    this, &SampleHookModule::OnCycleProcess, 0, HookType::HOOK_AFTER
                );

            }
        }

        virtual void Finalize()
        {
            // 'ReleaseParam()' must be called in 'Finalize()'.
            ReleaseParam();
        };

        // Parameter & Resource Map
        // These contents in this map are bound to the below XML.
        // This binding is defined in 'User/UserResourceMap.h' and 'User/UserInit.h'.
        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
            END_PARAM_PATH()
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
        END_RESOURCE_MAP()
    };

#ifdef USE_SAMPLE_HOOK_MODULE
    // Default parameters and connections for SampleHookModule.
    // This XML overwrite XML in DefaultParam.xml partially.
    static const char g_sampleHookModuleParam[] = "     \n\
                                                    \n\
    <?xml version='1.0' encoding='UTF-8'?>          \n\
    <Session>                                       \n\
      <Simulator>                                   \n\
        <Configurations>                            \n\
          <DefaultConfiguration>                    \n\
                                                    \n\
            <Structure>                             \n\
              <Copy>                                \n\
                <SampleHookModule                   \n\
                  Count = 'CoreCount'               \n\
                  Name  = 'sampleHookModule'        \n\
                />                                  \n\
              </Copy>                               \n\
            </Structure>                            \n\
                                                    \n\
            <Parameter>                             \n\
              <SampleHookModule                     \n\
                Name  = 'sampleHookModule'          \n\
              />                                    \n\
            </Parameter>                            \n\
          </DefaultConfiguration>                   \n\
        </Configurations>                           \n\
      </Simulator>                                  \n\
    </Session>                                      \n\
    ";

#endif  // #ifdef USE_SAMPLE_HOOK_MODULE

}

#endif  // #ifndef SAMPLES_SAMPLE_HOOK_MODULE_H
