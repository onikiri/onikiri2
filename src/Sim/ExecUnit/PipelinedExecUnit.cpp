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

#include "Sim/ExecUnit/PipelinedExecUnit.h"
#include "Sim/System/GlobalClock.h"
#include "Sim/Core/Core.h"
#include "Sim/ExecUnit/ExecLatencyInfo.h"

using namespace std;
using namespace Onikiri;

void PipelinedExecUnit::Execute( OpIterator op )
{
    ExecUnitBase::Execute( op );
    RegisterEvents( op, m_execLatencyInfo->GetLatency( op ) );
}

// Called in Evaluate phase.
bool PipelinedExecUnit::CanReserve( OpIterator op, int time )
{
    return m_reserver.CanReserve( 1, time, 1 );
}
        
// Called in Evaluate phase.
void PipelinedExecUnit::Reserve( OpIterator op, int time )
{
    m_numUsed++;
    m_reserver.Reserve( 1, time, 1 );
}

