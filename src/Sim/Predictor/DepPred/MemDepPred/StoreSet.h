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


#ifndef __STORESET_H__
#define __STORESET_H__

#include "Lib/shttl/setassoc_table.h"

#include "Interface/Addr.h"
#include "Env/Param/ParamExchange.h"

#include "Sim/Predictor/DepPred/MemDepPred/MemDepPredIF.h"
#include "Sim/Core/Core.h"
#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Op/OpContainer/OpExtraStateTable.h"
#include "Sim/Dependency/MemDependency/MemDependency.h"
#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/ISAInfo.h"
#include "Sim/Memory/AddrHasher.h"

namespace Onikiri {

    // Store Set Predictor
    class StoreSet :
        public MemDepPredIF,
        public PhysicalResourceNode
    {
    private:
        static const int WORD_BITS = SimISAInfo::INSTRUCTION_WORD_BYTE_SHIFT;
        static const u64 INVALID_STORESET_ID = 1;
        typedef AdrHasher HasherType;
        typedef Addr StoreSetID;
        typedef shttl::setassoc_table< std::pair<Addr, StoreSetID>, HasherType> StoreIDTableType;
        typedef shttl::setassoc_table< std::pair<Addr, OpIterator>, HasherType> ProducerTableType;

        Core* m_core;
        StoreIDTableType* m_storeIDTable;
        ProducerTableType* m_producerTable;
        OpExtraStateTable<StoreSetID> m_allocatedStoreIDTable;

        int m_numStoreIDTableEntryBits;
        int m_numStoreIDTableWays;
        int m_numProducerTableEntryBits;
        int m_numProducerTableWays;

        int m_numStoreIDTableEntries;
        int m_numProducerTableEntries;

        int m_numAccessOrderViolated;

        SharedPtrObjectPool<MemDependency> m_memDepPool;

        void AllocateMemDependency(OpIterator op);
        void ResolveMemDependency(OpIterator consumer, OpIterator producer);
        void Deallocate(const OpIterator op);

        void UpdateStoreSetIDTable(OpIterator op, StoreSetID id);
        void UpdateProducerTable(StoreSetID id, OpIterator op);
        void ReleaseProducerTable(OpIterator op);

        StoreSetID GetNewID(const OpIterator op);
        StoreSetID GetStoreSetID(const OpIterator op);
        OpIterator GetProducerStore(const StoreSetID id);


    public:
        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY("@StoreIDTableEntryBits",   m_numStoreIDTableEntryBits)
                PARAM_ENTRY("@StoreIDTableWays",        m_numStoreIDTableWays)
                PARAM_ENTRY("@ProducerTableEntryBits",  m_numProducerTableEntryBits)
                PARAM_ENTRY("@ProducerTableWays",       m_numProducerTableWays)
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
                PARAM_ENTRY( "@NumStoreSetIDTableEntries",    m_numStoreIDTableEntries)
                PARAM_ENTRY( "@NumProducerTableEntries",      m_numProducerTableEntries)
                PARAM_ENTRY( "@NumAccessOrderViolation",      m_numAccessOrderViolated)
            END_PARAM_PATH()
        END_PARAM_MAP()
        
        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( Core, "core", m_core )
        END_RESOURCE_MAP()

        using ParamExchange::LoadParam;
        using ParamExchange::ReleaseParam;

        StoreSet();
        virtual ~StoreSet();

        virtual void Initialize(InitPhase phase);

        virtual void Resolve(OpIterator op);
        virtual void Allocate(OpIterator op);
        virtual void Commit(OpIterator op);
        virtual void Flush(OpIterator op);

        virtual void OrderConflicted(OpIterator producer, OpIterator consumer);

        virtual bool CanAllocate(OpIterator* infoArray, int numOp);

    };

}; // namespace Onikiri

#endif // __STORESET_H__
