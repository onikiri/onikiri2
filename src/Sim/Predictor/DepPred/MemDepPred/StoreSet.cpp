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

#include "Sim/Predictor/DepPred/MemDepPred/StoreSet.h"
#include "Sim/Op/Op.h"

using namespace Onikiri;
using namespace boost;

StoreSet::StoreSet() :
    m_core(0),
    m_storeIDTable              (0),
    m_producerTable             (0),
    m_numStoreIDTableEntryBits  (0),
    m_numStoreIDTableWays       (0),
    m_numProducerTableEntryBits (0),
    m_numProducerTableWays      (0),
    m_numStoreIDTableEntries    (0),
    m_numProducerTableEntries   (0),
    m_numAccessOrderViolated    (0)
{
}

StoreSet::~StoreSet()
{
    if( m_storeIDTable ) {
        delete m_storeIDTable;
    }
    if( m_producerTable ) {
        delete m_producerTable;
    }

    // entry count
    m_numStoreIDTableEntries    = (1 << m_numStoreIDTableEntryBits);
    m_numProducerTableEntries   = (1 << m_numProducerTableEntryBits);
    ReleaseParam();
}

void StoreSet::Initialize(InitPhase phase)
{
    if( phase == INIT_PRE_CONNECTION ) {
        LoadParam();
    } else if( phase == INIT_POST_CONNECTION ) {
        m_storeIDTable = 
            new StoreIDTableType( 
                HasherType( m_numStoreIDTableEntryBits, WORD_BITS ),
                m_numStoreIDTableWays 
            );
        m_producerTable = 
            new ProducerTableType( 
                HasherType( m_numProducerTableEntryBits, WORD_BITS ), 
                m_numProducerTableWays 
            );
        m_allocatedStoreIDTable.Resize(*m_core->GetOpArray());
    }

}

void StoreSet::Resolve(OpIterator op)
{
    if( !op->GetOpClass().IsMem() ) {
        return;
    }

    StoreSetID storeSetID = GetStoreSetID(op);
    if( storeSetID.address != INVALID_STORESET_ID) {
        OpIterator producer = GetProducerStore(storeSetID);
        if( !producer.IsNull() ) {
            ResolveMemDependency(op, producer);
        }
    }
}

// allocate memory dependency to stores which caused Access Order Violation in the past 
void StoreSet::Allocate(OpIterator op)
{
    // allocate only stores
    if( !op->GetOpClass().IsStore() ) {
        return;
    }

    StoreSetID storeSetID = GetStoreSetID(op);
    if( storeSetID.address != INVALID_STORESET_ID) {
        // update tables and allocate dependency
        UpdateProducerTable(storeSetID, op);
        AllocateMemDependency(op);
    }
}

void StoreSet::Commit(OpIterator op)
{
    if( op->GetOpClass().IsMem() ) {
        Deallocate(op);
    }
}

void StoreSet::Flush(OpIterator op)
{
    if( op->GetStatus() == OpStatus::OS_FETCH ){
        return;
    }
    if( op->GetOpClass().IsMem() ) {
        Deallocate(op);
    }
}

// update Store Set on the event of Access Order Violation
void StoreSet::OrderConflicted(OpIterator producer, OpIterator consumer)
{
    ASSERT(producer->GetOpClass().IsStore(), "producerOp is not store(%s)", producer->ToString(6).c_str());
    ASSERT(consumer->GetOpClass().IsMem(), "consumerOp is not load/store(%s)", consumer->ToString(6).c_str());

    m_numAccessOrderViolated++;

    StoreSetID producerID = GetStoreSetID(producer);
    StoreSetID consumerID = GetStoreSetID(consumer);

    // update method varies depending on which Store Set IDs are present
    if( producerID.address == INVALID_STORESET_ID && consumerID.address == INVALID_STORESET_ID ) {
        // no entry exists, so create a new entry
        StoreSetID id = GetNewID(producer);
        UpdateStoreSetIDTable(producer, id);
        UpdateStoreSetIDTable(consumer, id);
    } else if( producerID.address != INVALID_STORESET_ID && consumerID.address == INVALID_STORESET_ID ) {
        // producer ID exists, so set consumer ID to the same number
        UpdateStoreSetIDTable(consumer, producerID);
    } else if( producerID.address == INVALID_STORESET_ID && consumerID.address != INVALID_STORESET_ID ) {
        // consumer ID exists, so set producer ID to the same number
        UpdateStoreSetIDTable(producer, consumerID);
    } else {
        if( producerID == consumerID ) {
            // same conflict occurred in the past
            return;
        }

        // if both IDs exist, compare them and set the bigger one to the smaller one
        if( producerID.address < consumerID.address ) {
            UpdateStoreSetIDTable(consumer, producerID);
        } else {
            // producer's table entry go out of use, so invalidate the entry
            ReleaseProducerTable(producer);
            UpdateStoreSetIDTable(producer, consumerID);
        }
    }
}

bool StoreSet::CanAllocate(OpIterator* infoArray, int numOp)
{
    return true;
}

void StoreSet::AllocateMemDependency(OpIterator op)
{
    if( op->GetDstMem(0) == NULL ) {
        MemDependencyPtr tmpMem(
            m_memDepPool.construct(m_core->GetNumScheduler()) );
        tmpMem->Clear();
        op->SetDstMem(0, tmpMem);
    }
}

void StoreSet::ResolveMemDependency(OpIterator consumer, OpIterator producer)
{
    ASSERT(producer->GetOpClass().IsStore(), "op is not store(%s)", producer->ToString(6).c_str());
    consumer->SetSrcMem(0, producer->GetDstMem(0));
}

void StoreSet::Deallocate(const OpIterator op)
{
    StoreSetID id = GetStoreSetID(op);
    if( id.address ) {
        ReleaseProducerTable(op);
        m_allocatedStoreIDTable[op] = StoreSetID();
    }
}

// update store set ID table and set op's ID
void StoreSet::UpdateStoreSetIDTable(OpIterator op, StoreSetID id)
{
    m_storeIDTable->write( op->GetPC(), id );
}

// assign a producer op to Store Set ID
void StoreSet::UpdateProducerTable(StoreSetID id, OpIterator op)
{
    m_producerTable->write(id, op);
    m_allocatedStoreIDTable[op] = id;
}

// deallocate producer table's entry if the op is the latest producer of the allocated Store Set ID
// (do nothing when new producer of the same ID was fetched)
void StoreSet::ReleaseProducerTable(OpIterator op)
{
    StoreSetID allocatedID = m_allocatedStoreIDTable[op];

    OpIterator lastProducer;
    bool producerFound = m_producerTable->read( allocatedID, &lastProducer ) != m_producerTable->end();
    if( producerFound ) {
        if( !lastProducer.IsNull() && lastProducer->GetGlobalSerialID() == op->GetGlobalSerialID() ){
            // the op is the latest producer of the allocatedID, so invalidate the table entry
            m_producerTable->write( allocatedID, OpIterator(0) );
        }
    }
}

// reserve producer table entry with op's PC(StoreSetID), and return new ID
StoreSet::StoreSetID StoreSet::GetNewID(const OpIterator op)
{
    StoreSetID id = op->GetPC();
    m_producerTable->write( op->GetPC(), OpIterator(0) );
    return id;
}

StoreSet::StoreSetID StoreSet::GetStoreSetID(const OpIterator op)
{
    StoreSetID id( op->GetPID(), op->GetTID(), INVALID_STORESET_ID );
    m_storeIDTable->read( op->GetPC(), &id );
    return id;
}

OpIterator StoreSet::GetProducerStore(const StoreSetID id)
{
    OpIterator producer;
    bool producerFound = m_producerTable->read( id, &producer ) != m_producerTable->end();
    if( producerFound && !producer.IsNull()) {
        return producer;
    } else {
        return OpIterator(0);
    }
}
