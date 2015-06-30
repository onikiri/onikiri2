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

#include "Sim/Pipeline/Scheduler/IssueSelector/AgeIssueSelector.h"
#include "Sim/Op/Op.h"

using namespace Onikiri;

AgeIssueSelector::AgeIssueSelector()
{

}

AgeIssueSelector::~AgeIssueSelector()
{

}

void AgeIssueSelector::Initialize( InitPhase phase )
{
}

void AgeIssueSelector::Finalize()
{
    ReleaseParam();
}

void AgeIssueSelector::EvaluateSelect( Scheduler* scheduler )
{
    // Select issued ops.
    int issueCount = 0;
    int issueWidth = scheduler->GetIssueWidth();

    const OpList& readyOps = scheduler->GetReadyOps();
    const SchedulingOps& wokeUpOps = scheduler->GetWokeUpOps();

    OpList::const_iterator          r = readyOps.begin();
    SchedulingOps::const_iterator   w = wokeUpOps.begin();

    while( issueCount < issueWidth ){
        OpIterator selected;

        // AgeIssueSelector selects the oldest ops from ready ops on issue.
        if( r != readyOps.end() && w != wokeUpOps.end() ){
            // There are both of ready ops and woke-up ops.
            if( (*r)->GetGlobalSerialID() < (*w)->GetGlobalSerialID() ){
                selected = *r;  ++r;
            }
            else{
                selected = *w;  ++w;
            }
        }
        else if( r != readyOps.end() ){
            // There are only ready ops.
            selected = *r;  ++r;
        }
        else if( w != wokeUpOps.end() ){
            // There are only woke-up ops.
            selected = *w;  ++w;
        }
        else{
            // No op can be selected.
            break;
        }

        if( scheduler->CanSelect( selected ) ) {
            scheduler->ReserveSelect( selected );
            ++issueCount;
        }
        else{
            g_dumper.Dump( DS_WAITING_UNIT, selected );
        }
    }
}
