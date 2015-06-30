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


#ifndef SIM_PREDICTOR_DEP_PRED_MEM_DEP_PRED_PERFECT_MEM_DEP_PRED_H
#define SIM_PREDICTOR_DEP_PRED_MEM_DEP_PRED_PERFECT_MEM_DEP_PRED_H

#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/Predictor/DepPred/MemDepPred/MemDepPredIF.h"
#include "Sim/Memory/MemOrderManager/MemOrderOperations.h"
#include "Sim/Dependency/MemDependency/MemDependency.h"

namespace Onikiri 
{
    class ForwardEmulator;
    class InorderList;

    // A perfect memory dependency predictor.
    class PerfectMemDepPred : 
        public MemDepPredIF,
        public PhysicalResourceNode 
    {

    public:
        BEGIN_PARAM_MAP( "" )
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( ForwardEmulator,    "forwardEmulator",  m_fwdEmulator )
            RESOURCE_ENTRY( InorderList,        "inorderList",      m_inorderList )
        END_RESOURCE_MAP()

        PerfectMemDepPred();
        virtual ~PerfectMemDepPred();

        virtual void Initialize( InitPhase phase );

        virtual void Resolve( OpIterator op );
        virtual void Allocate( OpIterator op );
        virtual void Commit( OpIterator op );
        virtual void Flush( OpIterator op );
        virtual void OrderConflicted( OpIterator producer, OpIterator consumer );
        virtual bool CanAllocate( OpIterator* infoArray, int numOp );
    
    protected:
        ForwardEmulator*    m_fwdEmulator;
        InorderList*        m_inorderList;
        MemOrderOperations  m_memOperations;
        SharedPtrObjectPool<MemDependency> m_memDepPool;
    };

}; // namespace Onikiri

#endif // SIM_PREDICTOR_DEP_PRED_MEM_DEP_PRED_MEM_DEP_PRED_H
