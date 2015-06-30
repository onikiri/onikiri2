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


#ifndef SIM_TIME_WHEEL_TIME_WHEEL_BASE_H
#define SIM_TIME_WHEEL_TIME_WHEEL_BASE_H

#include "Types.h"
#include "Env/Param/ParamExchange.h"
#include "Sim/Foundation/Event/EventList/EventList.h"
#include "Sim/Foundation/Event/EventList/EventWheel.h"
#include "Sim/Foundation/TimeWheel/ClockedResourceBase.h"

namespace Onikiri 
{
    
    //
    // タイムホイールの基底クラス
    // イベントを登録/処理する仕組み
    // 
    class TimeWheelBase : 
        public ParamExchange,
        public ClockedResourceBase
    {
    public:
        typedef EventPtr ListNode;
        typedef EventList ListType;

        BEGIN_PARAM_MAP( "/Session/Simulator/TimeWheelBase/" )
            PARAM_ENTRY("@Size",    m_size);
        END_PARAM_MAP()

        TimeWheelBase();
        virtual ~TimeWheelBase();

        // Add events
        virtual void AddEvent( const ListNode& evnt, int time );

        // Process events.
        virtual void End();

        // Proceed a time tick.
        virtual void Tick();

        // Get time tick count.
        // A difference between GetCycles() and GetNow is the following:
        //   GetCycles() returns the number of all experienced cycles, but 
        //   GetNow() returns the number of cycles that do not experience stall.
        //   A result of GetNow() is not changed in stall cycles.
        s64 GetNow() const;

        // Returns the size of the time wheel.
        int GetWheelSize() const
        {
            return m_size;
        }

        const ListType& EventsAfterTime(int time)
        {
            return *m_eventWheel.GetEventList( IndexAfterTime(time) ); 
        }

        const ListType& CurrentEvents()
        {
            return *m_eventWheel.GetEventList( m_current );
        }

    protected:

        // 各サイクル用のイベント
        EventWheel m_eventWheel;

        int IndexAfterTime( int time ) 
        {
            int index = m_current + time;
            while( index >= m_size ){
                index -= m_size;
            }
            return index; 
        }

    private:
        // m_event のどこを処理するか
        int m_current;
        
        // 確保するサイズ
        int m_size;

        // Time tick
        s64 m_now;

        // Copy is forbidden
        TimeWheelBase( const TimeWheelBase& ref ){}
        TimeWheelBase& operator=( const TimeWheelBase &ref ){   return(*this);  }
    };

}; // namespace Onikiri

#endif // SIM_TIME_WHEEL_TIME_WHEEL_BASE_H

