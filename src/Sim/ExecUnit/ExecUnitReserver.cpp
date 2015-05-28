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

#include "Sim/ExecUnit/ExecUnitReserver.h"

using namespace std;
using namespace Onikiri;

ExecUnitReserver::ExecUnitReserver() : 
    m_unitCount( 0 ),
    m_current( 0 )
{

}

ExecUnitReserver::~ExecUnitReserver()
{

}

void ExecUnitReserver::Initialize( int unitCount, int wheelSize )
{
    m_unitCount = unitCount;
    m_wheel.resize( wheelSize, 0 );
}

int ExecUnitReserver::GetWheelIndex( int delta )
{
    int index = (int)m_current + delta;
    int size = (int)m_wheel.size();
    if( index >= size ){
        index -= size;
    }
    return index;
}

void ExecUnitReserver::Begin()
{
    m_resvQueue.clear();
}

bool ExecUnitReserver::CanReserve( int n, int time, int period )
{
    for( int p = 0; p < period; ++p ){
        int now = time + p;
        int index = GetWheelIndex( now );
        int count = m_wheel[ index ];

        for( ReservationQueue::iterator i = m_resvQueue.begin(); i != m_resvQueue.end(); ++i ){
            int begin = i->time;
            int end   = begin + i->period;
            if( begin <= now && now < end ){
                count += i->count;
            }
        }

        if( count + n > m_unitCount ){
            return false;
        }
    }
    return true;
}

void ExecUnitReserver::Reserve( int n, int time, int period )
{
    Reservation res;
    res.count = n;
    res.time = time;
    res.period = period;
    m_resvQueue.push_back( res );
}

void ExecUnitReserver::Update()
{
    // Update the reservation wheel based on the reservation queue.
    for( ReservationQueue::iterator i = m_resvQueue.begin(); i != m_resvQueue.end(); ++i ){
        for( int cycle = 0; cycle < i->period; ++cycle ){
            int index = GetWheelIndex( i->time + cycle );
            m_wheel[ index ] += i->count;
            ASSERT( 
                m_wheel[ index ] <= m_unitCount, 
                "Execution units are reserved exceed a limit." 
            );
        }
    }

    // Reset a count of this cycle.
    m_wheel[ GetWheelIndex(0) ] = 0;

    m_current = GetWheelIndex(1);   // Get an index for a next cycle.
}
