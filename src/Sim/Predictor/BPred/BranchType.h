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


//
// Branch type for prediction
//

#ifndef __BRANCH_TYPE_H
#define __BRANCH_TYPE_H

namespace Onikiri
{
    class OpClass;
    
    enum BranchType
    {
        BT_CONDITIONAL = 0,
        BT_UNCONDITIONAL,
        BT_CALL,
        BT_RETURN,
        BT_CONDITIONAL_RETURN,
        BT_NON,
        BT_END              // dummy
    };

    class BranchTypeUtility
    {
    public:
        BranchType OpClassToBranchType(const OpClass& opClass) const;
        size_t     GetTypeCount() const;
        String     GetTypeName(size_t index) const;
    };
} // namespace Onikiri

#endif
