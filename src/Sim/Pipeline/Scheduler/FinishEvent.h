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


#ifndef SIM_PIPELINE_SCHEDULER_FINISH_EVENT_H
#define SIM_PIPELINE_SCHEDULER_FINISH_EVENT_H

#include "Sim/Foundation/Event/EventBase.h"
#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Foundation/Hook/HookDecl.h"

namespace Onikiri 
{
    // An event triggered on the execution finish of an op.
    class OpFinishEvent : public EventBase<OpFinishEvent>
    {
    public:

        struct FinishHookParam
        {
            // This flag is true when 'm_op' is flushed and is not alive.
            // In an execution finish phase, an op may be flushed by recovery from 
            // miss prediction and a hook method may be passed a dead op in this case.
            bool flushed;   
        };

        // Prototype: void OnFinish( OpIterator op, FinishParam* param );
        static HookPoint<OpFinishEvent, FinishHookParam> s_finishHook;

        OpFinishEvent( OpIterator op );
        virtual void Update();

    protected:
        OpIterator m_op;

    };


}; // namespace Onikiri

#endif // SIM_PIPELINE_SCHEDULER_FINISH_EVENT_H

