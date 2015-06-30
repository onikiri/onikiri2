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


#ifndef SIM_TIME_WHEEL_CLOCKED_RESOURCE_IF_H
#define SIM_TIME_WHEEL_CLOCKED_RESOURCE_IF_H

#include "Types.h"
#include "Sim/ResourcePriority.h"

namespace Onikiri 
{
    //
    class ClockedResourceIF
    {
    public:

        virtual ~ClockedResourceIF(){};
        
        //
        // --- These methods corresponds to 1-cycle behavior.
        // 
        // Decide what to do in Evaluate(), and update those in Update().
        //

        // Beginning of a cycle
        virtual void Begin()      = 0;  
        
        // Evaluate and decide to stall this cycle.
        // Processes with side-effects must not be done in this method.
        virtual void Evaluate()   = 0;  

        // Transition from Prepare and Process.
        // Processes with side-effects must not be done in this method.
        virtual void Transition() = 0;  

        // Update resources.
        // This method is not called when a module is stalled.
        // Processes observing other resources must not be done in this method.
        virtual void Update() = 0;

        // End of a cycle
        // Processes that observes other resources must not be done in this method.
        virtual void End()        = 0;  


        // Trigger Update() for internal user.
        virtual void TriggerUpdate() = 0;   


        // This method can be called only in Evaluate().
        virtual void StallThisCycle( ) = 0;

        // This method can be called in any phases.
        virtual void StallNextCycle( int cycle ) = 0;

        // Cancel a stall period set by StallNextCycle.
        virtual void CacnelStallPeriod() = 0;

        // Get a priority of this resource.
        // Priority constants are defined in "Sim/ResourcePriority.h".
        virtual int GetPriority() const = 0;

        // For debug
        virtual void SetParent( ClockedResourceIF* parent ) = 0;
        virtual const char* Who() const = 0;

    };

}; // namespace Onikiri

#endif // SIM_TIME_WHEEL_CLOCKED_RESOURCE_IF_H

