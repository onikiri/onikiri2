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


#ifndef __CONSERVATIVEMEMDEPPRED_H__
#define __CONSERVATIVEMEMDEPPRED_H__

#include "Sim/Predictor/DepPred/MemDepPred/MemDepPredIF.h"
#include "Sim/Dependency/MemDependency/MemDependency.h"
#include "Sim/Foundation/Checkpoint/CheckpointedData.h"

namespace Onikiri 
{
    class CheckpointMaster;

    // ÉÅÉÇÉäÇ…ä÷Ç∑ÇÈàÀë∂ä÷åWÇÃó\ë™äÌ
    // ç≈å„Ç…fetchÇ≥ÇÍÇΩêÊçsÇÃstoreñΩóﬂÇ…àÀë∂Ç∑ÇÈÇ∆ó\ë™Ç∑ÇÈ
    class ConservativeMemDepPred :
        public MemDepPredIF,
        public PhysicalResourceNode
    {
    private:
        Core* m_core;
        CheckpointMaster* m_checkpointMaster;
        CheckpointedData< MemDependencyPtr > m_latestStoreDst;
        CheckpointedData< MemDependencyPtr > m_latestMemDst;

        SharedPtrObjectPool<MemDependency> m_memDepPool;

        virtual void Deallocate(OpIterator op);

    public:
        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( CheckpointMaster, "checkpointMaster", m_checkpointMaster )
            RESOURCE_ENTRY( Core, "core", m_core )
        END_RESOURCE_MAP()

        ConservativeMemDepPred();
        virtual ~ConservativeMemDepPred();

        virtual void Initialize(InitPhase phase);

        virtual void Resolve(OpIterator op);
        virtual void Allocate(OpIterator op);
        virtual void Commit(OpIterator op);
        virtual void Flush(OpIterator op);

        virtual void OrderConflicted(OpIterator producer, OpIterator consumer);

        virtual bool CanAllocate(OpIterator* infoArray, int numOp);
    };

}; // namespace Onikiri

#endif // __CONSERVATIVEMEMDEPPRED_H__
