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

#include "Sim/Op/OpClassStatistics.h"
#include "Sim/Op/Op.h"

using namespace Onikiri;

// Constructor.
OpClassStatistics::OpClassStatistics()
{
    m_numOps.resize(OpClassCode::Code_MAX, 0);
    for(int i = 0; i < OpClassCode::Code_MAX; i++){
        m_names.push_back(OpClassCode::ToString(i));
    }
    Reset();
}

// Increment a counter corresponding to 'op'.
void OpClassStatistics::Increment(OpIterator op)
{
    const OpClass& opClass = op->GetOpClass();
    m_numOps[opClass.GetCode()]++;

    if(opClass.IsFloat()){
        m_numFpSrcOperands += op->GetSrcRegNum();
        m_numFpDstOperands += op->GetDstRegNum();
    }
    else{
        m_numIntSrcOperands += op->GetSrcRegNum();
        m_numIntDstOperands += op->GetDstRegNum();
    }
}

// Reset statistics counters.
void OpClassStatistics::Reset()
{
    for(size_t i = 0; i < m_numOps.size(); i++){
        m_numOps[i] = 0;
    }
    m_numIntSrcOperands = 0;
    m_numIntDstOperands = 0;
    m_numFpSrcOperands = 0;
    m_numFpDstOperands = 0;
}
