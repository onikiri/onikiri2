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


#ifndef SIM_RESOURCE_PRIORITY_H
#define SIM_RESOURCE_PRIORITY_H

#include "Types.h"

namespace Onikiri
{
    // Resources/events with higher priorities are processed earlier.
    // Priorities numbers must be continuous.
    enum ResourcePriority
    {
        RP_LOWEST = 0,      // Lowest priority

        // Commit
        // This priority must be lower than CRP_EXECUTION_FINISH.
        RP_COMMIT,

        // Execution finish.
        // This priority must be lower than CRP_DETECT_LATPRED_MISS.
        // (see CRP_DETECT_LATPRED_MISS comment)
        //
        // This priority must be lower than and CRP_DEFAULT_UPDATE,
        // because recovery process in execution finish must be done after
        // all usual update process..
        //
        RP_EXECUTION_FINISH,        

        // Detecting latency miss prediction.
        // This priority must be higher than that of 
        // execution finish process (CRP_DETECT_LATPRED_MISS). 
        RP_DETECT_LATPRED_MISS,     

        // Default priorities
        RP_DEFAULT_UPDATE,
        RP_DEFAULT_EVENT,

        // A 'Wakeup' priority must be higher than that of 'Select', which
        // is done in CRP_DEFAULT_UPDATE.
        RP_WAKEUP_EVENT,



        RP_HIGHEST          // Highest priority
    };
}

#endif // #define SIM_TIME_WHEEL_CLOCKED_RESOURCE_PRIORITY_H

