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


#include "pch.h"

#include "Sim/Pipeline/Scheduler/IssueSelector/IssueSelector.h"
#include "Sim/Op/Op.h"

using namespace Onikiri;

IssueSelector::IssueSelector() : 
    m_selector( NULL )
{

}

IssueSelector::~IssueSelector()
{

}

void IssueSelector::Initialize( InitPhase phase )
{
    // Do not call Initialize() of m_selector.
}


void IssueSelector::Finalize()
{
    ReleaseParam();
}


void IssueSelector::EvaluateSelect( Scheduler* scheduler )
{
    ASSERT( m_selector != NULL );
    m_selector->EvaluateSelect( scheduler );
}
