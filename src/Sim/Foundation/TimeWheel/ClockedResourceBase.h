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


#ifndef SIM_TIME_WHEEL_CLOCKED_RESOURCE_BASE_H
#define SIM_TIME_WHEEL_CLOCKED_RESOURCE_BASE_H

#include "Sim/Foundation/TimeWheel/ClockedResourceIF.h"

namespace Onikiri 
{
    
    class ClockedResourceBase : public ClockedResourceIF
    {
    public:
        enum PHASE
        {
            PHASE_BEGIN,
            PHASE_EVALUATE,
            PHASE_TRANSITION,
            PHASE_UPDATE,
            PHASE_END
        };

        struct ComparePriority 
        {
            bool operator()( const ClockedResourceIF* lhv, const ClockedResourceIF* rhv )
            { return lhv->GetPriority() > rhv->GetPriority(); }
        };

        ClockedResourceBase( const char* name = "" ) : 
            m_name( name ),
            m_reqStallThisCycle( false ),
            m_stallPeriod( 0 ),
            m_thisCycleStalled( false ),
            m_lastCycleStalled( false ),
            m_parent( NULL ),
            m_cycles( 0 ),
            m_stalledCycles( 0 ),
            m_phase( PHASE_BEGIN ),
            m_priority( RP_DEFAULT_UPDATE )
        {
        }

        ~ClockedResourceBase()
        {
        }

        //
        // These methods corresponds to 1-cycle behavior.
        //

        // Beginning of a cycle.
        virtual void Begin()
        {
            ASSERT( m_phase == PHASE_BEGIN );

            m_lastCycleStalled = m_thisCycleStalled;
            m_thisCycleStalled  = false;
            m_reqStallThisCycle = false;

            if( m_stallPeriod > 0 ) {
                m_reqStallThisCycle = true;
                --m_stallPeriod;
            }

            m_phase = PHASE_EVALUATE;

            Children::iterator end = m_children.end();
            for( Children::iterator i = m_children.begin(); i != end; ++i ){
                (*i)->Begin();
            }
            
            m_cycles++;
        }

        // Evaluate and decide to stall this cycle or not.
        virtual void Evaluate()
        {
            ASSERT( m_phase == PHASE_EVALUATE, "Do you call Begin() of a base class?" );
            m_phase = PHASE_TRANSITION;

            Children::iterator end = m_children.end();
            for( Children::iterator i = m_children.begin(); i != end; ++i ){
                (*i)->Evaluate();
            }
        }

        // Transition from Prepare and Process.
        // Fix whether stall is done at this cycle or not.
        virtual void Transition()
        {
            ASSERT( m_phase == PHASE_TRANSITION, "Do you call Evaluate() of a base class?"  );

            // Fix stall state
            m_thisCycleStalled = m_reqStallThisCycle;
            m_phase = PHASE_UPDATE;

            
            if( m_thisCycleStalled ){
                // Stall this cycle.
                m_stalledCycles++;
            }

            if( !m_lastCycleStalled && m_thisCycleStalled ){
                BeginStall();   
            }
            else if ( m_lastCycleStalled && !m_thisCycleStalled ){
                EndStall(); 
            }

            Children::iterator end = m_children.end();
            for( Children::iterator i = m_children.begin(); i != end; ++i ){
                (*i)->Transition();
            }
        }

        // Update resources.
        virtual void TriggerUpdate()
        {
            ASSERT( m_phase == PHASE_UPDATE, "Do you call Event() of a base class?" );

            if( !m_thisCycleStalled ){
                Update();
            }
            m_phase = PHASE_END;

            Children::iterator end = m_children.end();
            for( Children::iterator i = m_children.begin(); i != end; ++i ){
                (*i)->TriggerUpdate();
            }
        }

        // End of a cycle
        virtual void End()
        {
            ASSERT( m_phase == PHASE_END, "Do you call Process() of a base class?"  );
            m_phase = PHASE_BEGIN;

            Children::iterator end = m_children.end();
            for( Children::iterator i = m_children.begin(); i != end; ++i ){
                (*i)->End();
            }

            if( !m_thisCycleStalled ){
                // Proceed a time tick.
                Tick();
            }
        }

        // This method can be called only in Prepare().
        virtual void StallThisCycle()
        {
            ASSERT(
                m_phase == PHASE_EVALUATE || m_phase == PHASE_TRANSITION,
                "IsStalledThisCycle() can be called only in PHASE_EVALUATE/TRANSITION."
            );
            m_reqStallThisCycle = true;

            Children::iterator end = m_children.end();
            for( Children::iterator i = m_children.begin(); i != end; ++i ){
                (*i)->StallThisCycle();
            }
        }

        void StallThisCycleExcludingChildren()
        {
            ASSERT(
                m_phase == PHASE_EVALUATE || m_phase == PHASE_TRANSITION,
                "IsStalledThisCycle() can be called only in PHASE_EVALUATE/TRANSITION."
            );
            m_reqStallThisCycle = true;
        }

        // This method can be called in any phases.
        virtual void StallNextCycle( int cycles )
        {
            m_stallPeriod = std::max( cycles, m_stallPeriod );

            Children::iterator end = m_children.end();
            for( Children::iterator i = m_children.begin(); i != end; ++i ){
                (*i)->StallNextCycle( cycles );
            }
        }

        // Cancel a stall period set by StallNextCycle.
        virtual void CacnelStallPeriod()
        {
            m_stallPeriod = 0;

            Children::iterator end = m_children.end();
            for( Children::iterator i = m_children.begin(); i != end; ++i ){
                (*i)->CacnelStallPeriod();
            }
        }

        // Is this resource stalled in a this cycle
        bool IsStalledThisCycle()
        {
            ASSERT(
                m_phase == PHASE_UPDATE || m_phase == PHASE_END,
                "IsStalledThisCycle() can be called only in PHASE_PROCESS/END."
            );
            return m_thisCycleStalled;
        }

        // Is this resource stalled in a last cycle
        bool IsStalledLastCycle()
        {
            ASSERT(
                m_phase != PHASE_BEGIN,
                "IsStalledLastCycle() cannot be called in PHASE_BEGIN." 
            );
            return m_lastCycleStalled;
        }

        // Add a child resource
        virtual void AddChild( ClockedResourceIF* child )
        {
            m_children.push_back( child );
            child->SetParent( this );
        }

        // For debug
        virtual void SetParent( ClockedResourceIF* parent )
        {
            m_parent = parent;
            
            m_who = m_name + "(" + typeid(*this).name() + ")";
            if( m_parent ){
                m_who += String().format( " <= %s(Parent)  ", m_parent->Who() );
            }
        }

        virtual int GetPriority() const 
        {
            return m_priority;
        }

        virtual const char* Who() const
        {

            return m_who.c_str();
        }

        // This method is called in Process() if a resource is not stalled.
        virtual void Update(){};

        // Proceed a time tick.
        // This method is called in Transition() before BeginStall/End is called
        // when it is fixed to stall this cycle.
        virtual void Tick(){};

        // Stall handlers, which are called in stall begin/end
        virtual void BeginStall(){};
        virtual void EndStall(){};



    protected:

        // Child resources
        typedef std::vector<ClockedResourceIF*> Children;
        Children m_children;

        PHASE GetCurrentPhase() const
        {
            return m_phase;
        }

        s64 GetCycles() const
        {
            return m_cycles;
        }

        s64 GetStalledCycles() const
        {
            return m_stalledCycles;
        }

        // Priority constants are defined in "Sim/ResourcePriority.h".
        void SetPriority( int priority )
        {
            m_priority = priority;
        }


    private:
        String m_name;
        String m_who;

        // Stall request
        bool m_reqStallThisCycle;   
        
        // Stall period
        int  m_stallPeriod;
        
        // true if stall is done at a last cycle.
        bool m_thisCycleStalled;
        bool m_lastCycleStalled;

        ClockedResourceIF* m_parent;

        s64 m_cycles;

        // Stalled cycles.
        s64  m_stalledCycles;

        // Cycle phase
        PHASE m_phase;

        // Priority
        int m_priority;
    };

}; // namespace Onikiri

#endif // SIM_TIME_WHEEL_CLOCKED_RESOURCE_BASE_H

