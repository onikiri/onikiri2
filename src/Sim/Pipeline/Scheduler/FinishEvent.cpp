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

#include "Sim/Pipeline/Scheduler/FinishEvent.h"

#include "Utility/RuntimeError.h"
#include "Sim/Dumper/Dumper.h"
#include "Interface/Addr.h"

#include "Sim/Op/Op.h"
#include "Sim/Thread/Thread.h"
#include "Sim/Core/Core.h"
#include "Sim/InorderList/InorderList.h"
#include "Sim/Memory/MemOrderManager/MemOrderManager.h"
#include "Sim/Predictor/LatPred/LatPred.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/Pipeline/Fetcher/Fetcher.h"

#include "Sim/Foundation/Hook/Hook.h"
#include "Sim/Foundation/Hook/HookUtil.h"



using namespace Onikiri;

namespace Onikiri 
{
    HookPoint<OpFinishEvent,OpFinishEvent::FinishHookParam> OpFinishEvent::s_finishHook;
};


OpFinishEvent::OpFinishEvent( OpIterator op ) :
    m_op( op )
{
    SetPriority( RP_EXECUTION_FINISH );
}

void OpFinishEvent::Update()
{
    OpIterator op = m_op;
    ASSERT( 
        op->GetStatus() == OpStatus::OS_EXECUTING,
        "Op:%s",
        op->ToString().c_str()
    );

    FinishHookParam param;
    param.flushed = false;

    HOOK_SECTION_OP_PARAM( s_finishHook, op, param )
    {
        Core* core = op->GetCore();
        Thread* thread = op->GetThread();
        Scheduler* scheduler = m_op->GetScheduler();

        //
        // Notify the finish of execution to each module.
        //

        // Write back results to physical registers and update status.
        op->ExecutionEnd(); 

        // Each 'Finished' method may flush an op itself, 
        // so need to check whether an op is alive or not.

        // Update a latency predictor.
        if( op.IsAlive() ){
            core->GetLatPred()->Finished( op );
        }

        // To a fetcher.
        // In Fetcher::Finished(), Check branch miss prediction and
        // recover if it is necessary.
        if( op.IsAlive() ){
            core->GetFetcher()->Finished( op );
        }

        // To a memory order manager.
        // In MemOrderManager::Finished(), check access order violation  
        // and recover if violation occurs. 
        if( op.IsAlive() ){
            thread->GetMemOrderManager()->Finished( op );
        }

        // To a scheduler.
        if( op.IsAlive() ){
            scheduler->Finished( op );
        }

        if( !op.IsAlive() ){
            param.flushed = true;
        }

        // Value prediction is not implemented yet.
        //if( ValuePredictionMiss(m_op) ) {
        //  return m_op->GetThread()->RecoverValPredMiss(m_op, DataPredType::VALUE);
        //}
    }

}

