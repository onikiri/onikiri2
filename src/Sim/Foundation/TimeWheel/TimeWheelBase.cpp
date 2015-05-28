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

#include <utility>

#include "Env/Env.h"
#include "Sim/Dumper/Dumper.h"

#include "Sim/Foundation/TimeWheel/TimeWheelBase.h"

using namespace std;
using namespace Onikiri;


TimeWheelBase::TimeWheelBase() :
    m_current(0),
    m_size(0),
    m_now(0)
{
    LoadParam();
    m_eventWheel.Resize( m_size );  
}

TimeWheelBase::~TimeWheelBase()
{
    ReleaseParam();
}

void TimeWheelBase::AddEvent( const ListNode& evnt, int time )
{
    ASSERT(
        ( GetCurrentPhase() == PHASE_UPDATE || 
          GetCurrentPhase() == PHASE_END 
        ),
        "AddEvent() can be called only in PHASE_PROCESS/END."
    );
    ASSERT( time >= 0, "time is negative." );
    ASSERT( m_size > time, "event list is not enough." );
    
    if( /*m_processedThisCycle &&*/ time == 0 ){
        evnt->TriggerUpdate();
        return;
    }

    //ASSERT( !(time == 0 && m_processedThisCycle == true) , "event added after process.");

    int id = IndexAfterTime( time );
    m_eventWheel.GetEventList( id )->AddEvent( evnt );
}


// Proceed a time tick
void TimeWheelBase::Tick()
{
    m_current++;
    if(m_current >= m_size){
        m_current -= m_size;
    }
    ++m_now;
}


// Get time tick count.
s64 TimeWheelBase::GetNow() const
{
    ASSERT(
        ( GetCurrentPhase() == PHASE_UPDATE || 
          GetCurrentPhase() == PHASE_END 
        ),
        "GetNow() can be called only in PHASE_PROCESS/END."
    );
    return m_now;
}

// Process events.
void TimeWheelBase::End()
{
    if( !IsStalledThisCycle() ){
        // Clear events
        int current = m_current;
        ListType* curEvent = m_eventWheel.PeekEventList(current);
        if( curEvent ){
            curEvent->Clear();
            m_eventWheel.ReleaseEventList( curEvent, current );
        }
    }

    ClockedResourceBase::End();
}

