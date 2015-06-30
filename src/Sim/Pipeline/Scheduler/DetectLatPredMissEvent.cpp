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

#include "Utility/RuntimeError.h"
#include "Sim/Pipeline/Scheduler/DetectLatPredMissEvent.h"
#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Thread/Thread.h"
#include "Sim/Dumper/Dumper.h"
#include "Sim/Op/Op.h"
#include "Sim/InorderList/InorderList.h"
#include "Sim/Recoverer/Recoverer.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"

using namespace Onikiri;

OpDetectLatPredMissEvent::OpDetectLatPredMissEvent(
    const OpIterator& op, 
    int level,
    int predicted,
    int latency
) :
    m_op(op),
    m_level(level),
    m_predicted(predicted),
    m_latency(latency)
{
    // Detecting latency miss prediction.
    // This priority must be higher than that of execution finish process. 
    SetPriority( RP_DETECT_LATPRED_MISS );
}

OpDetectLatPredMissEvent::~OpDetectLatPredMissEvent()
{
}


void OpDetectLatPredMissEvent::Update()
{
    OpIterator op = m_op;
    Scheduler* scheduler = m_op->GetScheduler();
    if( m_predicted < m_latency ){

        if( op->IsSrcReady( scheduler->GetIndex() ) ){
            // Re-scheduling wakeup of consumers
            Recoverer* recoverer = op->GetThread()->GetRecoverer();
            recoverer->RecoverDataPredMiss( 
                op,
                op->GetFirstConsumer(), 
                DataPredMissRecovery::TYPE_LATENCY 
            );
            g_dumper.Dump( DS_LATENCY_PREDICTION_MISS, op );
        }
    }

}

