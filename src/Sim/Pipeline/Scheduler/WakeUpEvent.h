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


#ifndef SIM_PIPELINE_SCHEDULER_WAKE_UP_EVENT_H
#define SIM_PIPELINE_SCHEDULER_WAKE_UP_EVENT_H

#include "Sim/Foundation/Event/EventBase.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/Dependency/Dependency.h"
#include "Sim/Dependency/PhyReg/PhyReg.h"
#include "Sim/Dependency/MemDependency/MemDependency.h"
#include "Sim/Op/Op.h"
#include "Sim/Dumper/Dumper.h"


namespace Onikiri 
{

    class OpWakeUpEvent : public EventBase< OpWakeUpEvent >
    {
    public:
        using EventBase< OpWakeUpEvent >::SetPriority;
    
        OpWakeUpEvent( Scheduler* scheduler, OpIterator producer ) :
            m_scheduler ( scheduler ),
            m_producer  ( producer )
        {
            // A 'Wakeup' priority must be higher than that of 'Select'. 
            SetPriority( RP_WAKEUP_EVENT );
            m_evaluated = false;
        }

        virtual void Evaluate()
        {
            m_evaluated = true;

            int index = m_scheduler->GetIndex();
            for( int i = 0; i < m_producer->GetDstRegNum(); ++i ){
                if( m_producer->GetDstPhyReg(i)->GetReadiness( index ) ){
                    return; // This dependency is already set to true and consumers are woke up.
                }
            }

            m_scheduler->EvaluateDependency( m_producer );
        }

        virtual void Update()
        {
            ASSERT( m_evaluated );
        };

    protected:

        Scheduler*  m_scheduler;
        OpIterator  m_producer;
        bool        m_evaluated;
    };

}; // namespace Onikiri

#endif // SIM_PIPELINE_SCHEDULER_WAKE_UP_EVENT_H

