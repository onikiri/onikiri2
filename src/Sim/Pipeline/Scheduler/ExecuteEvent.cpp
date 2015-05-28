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

#include "Sim/Pipeline/Scheduler/ExecuteEvent.h"

#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/ExecUnit/ExecUnitIF.h"
#include "Sim/Foundation/Hook/Hook.h"
#include "Sim/Foundation/Hook/HookUtil.h"
#include "Sim/Op/Op.h"

using namespace std;
using namespace Onikiri;


namespace Onikiri 
{
    HookPoint<OpExecuteEvent> OpExecuteEvent::s_executeHook;
};  // namespace Onikiri

OpExecuteEvent::OpExecuteEvent( OpIterator op ) : m_op( op )
{
    // OpExecuteEvent may invoke OpDetectLatPredMissEvent and OpFinishEvent, 
    // so this event's priority must be set OpFinishEvent. 
    SetPriority( RP_EXECUTION_FINISH );
}

void OpExecuteEvent::Update()
{
    HOOK_SECTION_OP( s_executeHook, m_op ){
        // 演算器に実行終了のタイミングで FinishEvent を追加してもらう
        m_op->GetExecUnit()->Execute( m_op );
    }
}
