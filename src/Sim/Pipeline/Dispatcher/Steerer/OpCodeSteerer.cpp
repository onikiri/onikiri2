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

#include "Sim/Pipeline/Dispatcher/Steerer/OpCodeSteerer.h"
#include "Sim/Op/Op.h"
#include "Sim/Core/Core.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/ExecUnit/ExecUnitIF.h"

using namespace Onikiri;
using namespace std;

OpCodeDispatchSteerer::OpCodeDispatchSteerer()
{
    m_core = 0;
}

OpCodeDispatchSteerer::~OpCodeDispatchSteerer()
{
    ReleaseParam();
}

void OpCodeDispatchSteerer::Initialize(InitPhase phase)
{
    if (phase == INIT_PRE_CONNECTION){
        LoadParam();
        return;
    }
    if (phase == INIT_POST_CONNECTION){

        CheckNodeInitialized( "core", m_core );

        for(int i = 0; i < m_core->GetNumScheduler(); ++i) {
            Scheduler* sched = m_core->GetScheduler(i);
            const vector<ExecUnitIF*>& unitList = 
                sched->GetExecUnitList();

            for( size_t i = 0; i < unitList.size(); i++){
                int codeCount = unitList[i]->GetMappedCodeCount();
                for(int j = 0; j < codeCount; j++){
                    int code = unitList[i]->GetMappedCode( j );

                    // code が末尾のインデックスになるように拡張
                    if((int)m_schedulerMap.size() <= code)
                        m_schedulerMap.resize(code+1);

                        ASSERT( m_schedulerMap[code] == 0, "scheduler set twice(code:%d).", code);

                        // 該当する番号に代入
                        m_schedulerMap[code] = sched;
                }
            }
        }
    }

}


Scheduler* OpCodeDispatchSteerer::Steer(OpIterator opIterator)
{
    int code = opIterator->GetOpClass().GetCode();

    ASSERT( code >= 0 && code < static_cast<int>(m_schedulerMap.size()),
        "unknown opcode %d.", code);
    ASSERT( m_schedulerMap[code] != 0,
        "scheduler not set(opcode %d).", code);

    return m_schedulerMap[code];
}
