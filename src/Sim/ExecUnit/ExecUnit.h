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


#ifndef SIM_EXEC_UNIT_EXEC_UNIT_H
#define SIM_EXEC_UNIT_EXEC_UNIT_H

#include "Sim/ExecUnit/ExecUnitBase.h"
#include "Env/Param/ParamExchange.h"
#include "Sim/Op/OpArray/OpArray.h"

namespace Onikiri
{

    // Execution unit(not pipelined)
    class ExecUnit : public ExecUnitBase
    {

    public:
        BEGIN_PARAM_MAP("")
            CHAIN_BASE_PARAM_MAP( ExecUnitBase )
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            CHAIN_BASE_RESOURCE_MAP( ExecUnitBase )
        END_RESOURCE_MAP()

        virtual void Execute( OpIterator op );

        // Called in Evaluate phase.
        virtual bool CanReserve( OpIterator op, int time );
        
        // Called in Evaluate phase.
        virtual void Reserve( OpIterator op, int time );
    };

}; // namespace Onikiri

#endif // __EXECUNIT_H__

