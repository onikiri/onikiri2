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


#ifndef SIM_SYSTEM_SIMULATION_SYSTEM_SIMULATION_SYSTEM_H
#define SIM_SYSTEM_SIMULATION_SYSTEM_SIMULATION_SYSTEM_H

#include "Sim/System/SystemBase.h"
#include "Sim/Pipeline/Pipeline.h"
#include "Sim/Memory/Cache/Cache.h"
#include "Sim/Foundation/Event/PriorityEventList.h"

namespace Onikiri 
{
    class SimulationSystem : public SystemBase
    {

    public:
        SimulationSystem();
        void Run( SystemContext* context );
        SystemContext* GetContext();

        // Prototype : void Method( HookParameter<SimulationSystem>* system )
        static HookPoint<SimulationSystem, SimulationSystem> s_systemInitHook;

        // Prototype : void Method()
        static HookPoint<SimulationSystem> s_cycleBeginHook;
        static HookPoint<SimulationSystem> s_cycleEvaluateHook;
        static HookPoint<SimulationSystem> s_cycleTransitionHook;
        static HookPoint<SimulationSystem> s_cycleUpdateHook;
        static HookPoint<SimulationSystem> s_cycleEndHook;

    protected:
        struct SchedulerResources
        {
        };

        struct CoreResources
        {
            Core*     core;
        };

        struct ThreadResources
        {
            MemOrderManager* memOrderManager;
        };

        struct MemoryResources
        {
        };

        std::vector<CoreResources>   m_coreResources;
        std::vector<ThreadResources> m_threadResources;
        std::vector<MemoryResources> m_memResources;

        typedef std::vector<ClockedResourceIF*> ClockedResourceList;
        ClockedResourceList m_clockedResources;

        typedef PriorityEventList::TimeWheelList TimeWheelList;
        TimeWheelList m_timeWheels;

        PriorityEventList m_priorityEventList;

        void SimulateCycle();

        void CycleBegin();
        void CycleEvaluate();
        void CycleTransition();
        void CycleEvent();
        void CycleUpdate();
        void CycleEnd();

        void InitializeResources();
        void InitializeResourcesBody();

        SystemContext* m_context;
    };
}; // namespace Onikiri

#endif // SIM_SYSTEM_SIMULATION_SYSTEM_SIMULATION_SYSTEM_H

