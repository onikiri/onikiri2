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



#ifndef SIM_FOUNDATION_EVENT_PRIORITY_EVENT_LIST_H
#define SIM_FOUNDATION_EVENT_PRIORITY_EVENT_LIST_H

#include "Sim/Foundation/Event/EventBase.h"
#include "Sim/Foundation/TimeWheel/TimeWheelBase.h"
#include "Sim/ResourcePriority.h"
#include "Utility/Collection/fixed_size_buffer.h"

namespace Onikiri
{
    class PriorityEventList
    {
    public:
        PriorityEventList() :
            m_updatePriority( RP_LOWEST ),
            m_evaluatePriority( RP_LOWEST )
        {
            //m_eventList.resize( CRP_HIGHEST + 1 );
            for( int i = RP_LOWEST; i < RP_HIGHEST + 1; i++ ){
                m_eventList.push_back( new EventList );
            }
        }

        ~PriorityEventList()
        {
            for( PriorityList::iterator i = m_eventList.begin(); i != m_eventList.end(); ++i ){
                delete *i;
            }
            m_eventList.clear();
        }

        //typedef EventPtr PtrType;
        typedef EventBaseImplement* PtrType;
        struct Entry
        {
            PtrType event;
            TimeWheelBase* timeWheel;

            Entry( const PtrType& e, TimeWheelBase* t ) :
                event( e ),
                timeWheel( t )
            {

            }
            Entry() :
                event( 0 ),
                timeWheel( 0 )
            {

            }
        };

        //typedef pool_vector< Entry > EventList;
        enum MaxEvent{ MAX_EVENT_IN_ONE_CYCLE = 4096 };
        typedef fixed_sized_buffer< Entry, MAX_EVENT_IN_ONE_CYCLE, MaxEvent > EventList;

        enum MaxPriority{ MAX_PRIORITY = RP_HIGHEST + 1 };
        typedef fixed_sized_buffer< EventList*, MAX_PRIORITY, MaxPriority > PriorityList;

        typedef std::vector< TimeWheelBase* > TimeWheelList;

        // Extract events processed in this cycle.
        // Events are extracted from 'timeWheels' to 'm_eventList'.
        void ExtractEvent( TimeWheelList* timeWheels )
        {
            for( PriorityList::iterator i = m_eventList.begin(); i != m_eventList.end(); ++i ){
                (*i)->clear();
            }

            // Extract events triggered in this cycle.
            TimeWheelList::iterator timeEnd = timeWheels->end();
            for( TimeWheelList::iterator i = timeWheels->begin(); i != timeEnd; ++i ){
                TimeWheelBase* wheel = *i;
                const TimeWheelBase::ListType& events = wheel->CurrentEvents();
                TimeWheelBase::ListType::ConstIteratorType end = events.end();
                for( TimeWheelBase::ListType::ConstIteratorType e = events.begin(); e != end; ++e ){
                    PtrType event = &(*e->event);
                    if( !event->IsCanceled() ){
                        EventList* list = m_eventList[ event->GetPriority() ];
                        list->push_back( Entry( event, wheel ) );
                    }
                }
            }

        }

        void BeginEvaluate()
        {
            m_evaluatePriority = RP_HIGHEST;
        }

        void TriggerEvaluate( int priority )
        {
            int curPriority;
            for( curPriority = m_evaluatePriority; curPriority >= priority; curPriority-- ){
                EventList* eventList = m_eventList[curPriority];
                EventList::iterator eventEnd = eventList->end();
                for( EventList::iterator i = eventList->begin(); i != eventEnd; ++i ){
                    i->event->TriggerEvaluate();
                }
            }
            m_evaluatePriority = curPriority; 
        }

        void EndEvaluate()
        {
            TriggerEvaluate( RP_LOWEST );
        }


        void BeginUpdate()
        {
            m_updatePriority = RP_HIGHEST;
        }

        void TriggerUpdate( int priority )
        {
            int curPriority;
            for( curPriority = m_updatePriority; curPriority >= priority; curPriority-- ){
                EventList* eventList = m_eventList[curPriority];
                EventList::iterator eventEnd = eventList->end();
                for( EventList::iterator i = eventList->begin(); i != eventEnd; ++i ){
                    if( !i->timeWheel->IsStalledThisCycle() ){
                        i->event->TriggerUpdate();
                    }
                }
            }
            m_updatePriority = curPriority; 
        }

        void EndUpdate()
        {
            TriggerUpdate( RP_LOWEST );
        }

    private:

        PriorityList m_eventList;
        int m_updatePriority;
        int m_evaluatePriority;
    };

} //namespace Onikiri



#endif

