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
#include "Sim/Predictor/BPred/BranchType.h"

using namespace Onikiri;

size_t BranchTypeUtility::GetTypeCount() const
{
    return BT_END - BT_CONDITIONAL;
}

BranchType BranchTypeUtility::OpClassToBranchType(const OpClass& opClass) const
{
    if(!opClass.IsBranch())
        return BT_NON;

    bool conditinal = opClass.IsConditionalBranch();

    if( opClass.IsCall() )
        return BT_CALL;
    else if( opClass.IsReturn() )
        return conditinal ? BT_CONDITIONAL_RETURN : BT_RETURN;
    else if( conditinal )
        return BT_CONDITIONAL;
    else
        return BT_UNCONDITIONAL;
}

String BranchTypeUtility::GetTypeName(size_t index) const
{
    const char* name[] = 
    {
        "Conditional",
        "Unconditional",
        "Call",
        "Return",
        "Conditional return",
        "Not branch"
    };

    BranchType type = (BranchType)index;
    if(type == BT_END){
        THROW_RUNTIME_ERROR("BT_END is invalid.");
    }
    return name[type];
}
