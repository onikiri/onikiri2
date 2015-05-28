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
#include "Sim/System/ForwardEmulator.h"

#include "Sim/Thread/Thread.h"
#include "Sim/Core/Core.h"
#include "Sim/Op/Op.h"
#include "Sim/Foundation/SimPC.h"

using namespace Onikiri;

ForwardEmulator::ForwardEmulator() : 
    m_emulator( NULL ),
    m_enable( false )
{
}

ForwardEmulator::~ForwardEmulator()
{
}

void ForwardEmulator::Initialize( InitPhase phase )
{
    if( phase == INIT_PRE_CONNECTION ){
        LoadParam();
    }
    else if( phase == INIT_POST_CONNECTION ){
        m_memOperations.SetTargetEndian( m_emulator->GetISAInfo()->IsLittleEndian() );
    }
}

void ForwardEmulator::Finalize()
{
    ReleaseParam();
}

void ForwardEmulator::SetContext( const ArchitectureStateList& context )
{
    m_context = context;
    m_inflightOps.clear();
    m_inflightOps.resize( m_context.size() );

    m_threadContext.resize( m_context.size() );
    for( size_t i = 0; i < m_context.size(); ++i ){
        m_threadContext[i].nextFixedPC = context[i].pc;
    }
}

void ForwardEmulator::OnFetch( OpIterator simOp )
{
    if( !m_enable )
        return;

    int tid = simOp->GetTID();
    ArchitectureState* context  = &m_context[ tid ];
    InflightOpList* inflightOps = &m_inflightOps[ tid ];

    // Return when emulation is already done on recovery 
    // from branch miss prediction.
    if( inflightOps->size() > 0 ){
        InflightOp* entry = &inflightOps->back();
        if( entry->retireId >= simOp->GetRetireID() ){
            // Already executed in ForwardEmulator.
            UpdateFixedPath( simOp );
            return;
        }

        // System calls are treated specially.
        // The execution of system calls is delayed to OnCommit, because
        // executing system calls immediately affect architecture state such as memory image.
        // A system call may be divided to multiple micro ops and second or later divided ops
        // are not pushed to 'm_inflightOps', so PCs do not match.
        if( entry->emuOp.GetOpInfo()->GetOpClass().IsSyscall() &&
            entry->emuOp.GetPC() != simOp->GetPC()
        ){
            return;
        }
    }


    //
    // Execution
    //

    inflightOps->push_back( InflightOp() );
    InflightOp* entry = &inflightOps->back();
    entry->simOp    = simOp;
    entry->retireId = simOp->GetRetireID();

    // Get OpInfo
    std::pair<OpInfo**, int> ops = 
        m_emulator->GetOp( context->pc );
    OpInfo** opInfoArray = ops.first;
    int opCount          = ops.second;
    ASSERT( context->microOpIndex < opCount );
    OpInfo* opInfo = opInfoArray[ context->microOpIndex ];
    entry->updatePC = context->microOpIndex >= opCount - 1;
    entry->isStore  = opInfo->GetOpClass().IsStore();

    // Initialize an emu op.
    EmulationOp* emuOp = &entry->emuOp;
    emuOp->SetMem( this );
    emuOp->SetPC( context->pc );
    emuOp->SetTakenPC( Addr( tid, simOp->GetTID(), context->pc.address + SimISAInfo::INSTRUCTION_WORD_BYTE_SIZE ) );
    emuOp->SetOpInfo( opInfo );
    emuOp->SetTaken( false );
    // Set source operands.
    int srcCount = opInfo->GetSrcNum();
    for( int i = 0; i < srcCount; i++ ) {
        emuOp->SetSrc(i, context->registerValue[ opInfo->GetSrcOperand( i ) ] );
    }

    // Execution
    if( opInfo->GetOpClass().IsSyscall() ){
        // System calls are treated specially.
        // The execution of system calls is delayed to OnCommit, because
        // executing system calls immediately affect architecture state such as memory image.
        entry->updateMicroOpIndex = false;
        context->microOpIndex++;
        return;
    }
    else{
        m_emulator->Execute( emuOp, opInfo );
        UpdateArchContext( context, emuOp, opInfo, entry->updatePC, true );
        UpdateFixedPath( simOp );
    }
}

void ForwardEmulator::UpdateArchContext( 
    ArchitectureState* context, 
    OpStateIF* state, 
    OpInfo* info, 
    bool updatePC, 
    bool updateMicroOpIndex  
){
    // Get results.
    int dstCount = info->GetDstNum();
    for (int i = 0; i < dstCount; i ++) {
        context->registerValue[ info->GetDstOperand(i) ] = state->GetDst(i);
    }

    // Update PCs
    if( updateMicroOpIndex )
        context->microOpIndex++;

    if( updatePC ){
        context->microOpIndex = 0;
        context->pc = GetExecutedNextPC( context->pc, state );
    }
}


void ForwardEmulator::OnCommit( OpIterator simOp )
{
    if( !m_enable )
        return;

    int tid = simOp->GetTID();
    InflightOpList* inflightOps = &m_inflightOps[ tid ];
    InflightOpList::iterator front = inflightOps->begin();
    
    ASSERT(
        front != inflightOps->end(),
        "A list of in-flight ops is empty." 
    );
    ASSERT(
        front->retireId == simOp->GetRetireID(),
        "A retire-id of a forwardly emulated op (%lld) and that of a simulated op (%lld) are inconsistent.",
        front->retireId,
        simOp->GetRetireID()
    );

    EmulationOp* emuOp = &( front->emuOp );

    const OpInfo* emuOpInfo = emuOp->GetOpInfo();
    const OpClass& emuOpClass = emuOpInfo->GetOpClass();

    if( emuOpClass.IsSyscall() ){
        // System calls are treated specially.
        // The execution of system calls is delayed to OnCommit, because
        // executing system calls immediately affect architecture state such as memory image.
        ArchitectureState* context  = &m_context[ tid ];
        UpdateArchContext( 
            context,
            simOp.GetOp(), 
            simOp->GetOpInfo(), 
            front->updatePC, 
            front->updateMicroOpIndex 
        );
        UpdateFixedPath( simOp );
        //ASSERT( inflightOps->size() == 1, "System call must be serialized." );
    }
    else{
        u64 simDst = simOp->GetDstRegNum() > 0 ? simOp->GetDst(0) : 0;
        u64 emuDst = emuOpInfo->GetDstNum() > 0 ? emuOp->GetDst(0) : 0;
        u64 simPC  = simOp->GetPC().address;
        u64 emuPC  = emuOp->GetPC().address;
        u64 simAddr = emuOpClass.IsMem() ? simOp->GetMemAccess().address.address : 0;
        u64 emuAddr = emuOpClass.IsMem() ? front->memAccess.address.address : 0;

        if( simDst != emuDst || simPC != emuPC || simAddr != emuAddr ){
            THROW_RUNTIME_ERROR( 
                "Simulation result is incorrect.\n" 
                "Forward emulation result:  '%08x%08x'\n"
                "Simulation result:         '%08x%08x'\n"
                "Forward emulation pc:      '%08x%08x'\n"
                "Simulation pc:             '%08x%08x'\n"
                "Forward emulation address: '%08x%08x'\n"
                "Simulation address:        '%08x%08x'\n"
                "%s\n%s",
                (u32)(emuDst >> 32), (u32)(emuDst & 0xffffffff),
                (u32)(simDst >> 32), (u32)(simDst & 0xffffffff),
                (u32)(emuPC >> 32),  (u32)(emuPC & 0xffffffff),
                (u32)(simPC >> 32),  (u32)(simPC & 0xffffffff),
                (u32)(emuAddr >> 32),  (u32)(emuAddr & 0xffffffff),
                (u32)(simAddr >> 32),  (u32)(simAddr & 0xffffffff),
                simOp->ToString().c_str(),
                simOp->GetOpInfo()->GetMnemonic()
            );
        }
    }



    inflightOps->pop_front();
}


// Get a pre-executed result corresponding to 'op'
const OpStateIF* ForwardEmulator::GetExecutionResult( OpIterator op )
{
    InflightOp* ifOp = GetInflightOp( op );
    return (ifOp != NULL ) ? &ifOp->emuOp : NULL;
}

const MemAccess* ForwardEmulator::GetMemoryAccessResult( OpIterator op )
{
    InflightOp* ifOp = GetInflightOp( op );
    return (ifOp != NULL ) ? &ifOp->memAccess : NULL;
}

// Returns whether 'op' is in miss predicted path or not.
bool ForwardEmulator::IsInMissPredictedPath( OpIterator op )
{
    int tid = op->GetTID();
    ThreadContext* threadContext = &m_threadContext[ tid ];
    return ( op->GetRetireID() >= threadContext->nextFixedRetireID ) ? true : false;
}

void ForwardEmulator::Read( MemAccess* access )
{
    int tid = access->address.tid;
    InflightOpList* inflightOps = &m_inflightOps[ tid ];
    InflightOp* entry = &inflightOps->back();

    entry->memAccess = *access;
    InflightOp* producer = GetProducerStore( *entry );

    bool partial = false;
    if( producer != NULL ){
        if( m_memOperations.IsInnerAccess( *access, producer->memAccess ) ){
            access->value = m_memOperations.ReadPreviousAccess( *access, producer->memAccess );
            entry->memAccess = *access;
            return;
        }
        else{
            partial = true;
            //THROW_RUNTIME_ERROR( "A partial read access in ForwardEmulator is not supported yet." );
        }
    }

    if( partial ){

        // Merge multiple stores to one load.
        MemAccess base = *access;
        m_emulator->GetMemImage()->Read( &base );

        for( InflightOpList::iterator i = inflightOps->begin(); i != inflightOps->end(); ++i ){
            if( !i->isStore ){
                continue;
            }

            if( m_memOperations.IsOverlapped( base, i->memAccess ) ){
                base.value = 
                    m_memOperations.MergePartialAccess( base, i->memAccess );
            }
        }

        *access = base;
        entry->memAccess = base;
        return;
    }

    m_emulator->GetMemImage()->Read( access );
    entry->memAccess = *access;
}

void ForwardEmulator::Write( MemAccess* access )
{
    int tid = access->address.tid;
    InflightOpList* inflightOps = &m_inflightOps[ tid ];
    InflightOp* op = &inflightOps->back();
    op->memAccess = *access;
}

// When 'reverse' is true, an in-flight op is searched from the back of 'inflightOps'.
ForwardEmulator::InflightOp* ForwardEmulator::GetInflightOp( OpIterator op, bool reverse )
{
    int tid = op->GetTID();
    InflightOpList* inflightOps = &m_inflightOps[ tid ];

    u64 retireID = op->GetRetireID();

    if( reverse ){
        for( InflightOpList::reverse_iterator i = inflightOps->rbegin(); i != inflightOps->rend(); ++i ){
            if( i->retireId == retireID ){
                return &(*i);
            }
        }
    }
    else{
        for( InflightOpList::iterator i = inflightOps->begin(); i != inflightOps->end(); ++i ){
            if( i->retireId == retireID ){
                return &(*i);
            }
        }
    }
    return NULL;
}

ForwardEmulator::InflightOp*
    ForwardEmulator::GetProducerStore( const InflightOp& consumerLoad )  
{
    const MemAccess& access = consumerLoad.memAccess;
    int tid = access.address.tid;
    InflightOpList* inflightOps = &m_inflightOps[ tid ];

    InflightOpList::reverse_iterator end = inflightOps->rend();
    for( InflightOpList::reverse_iterator i = inflightOps->rbegin(); i != end; ++i ){
        if( !i->isStore ){
            continue;
        }
        if( consumerLoad.retireId < i->retireId ){
            continue;
        }

        if( m_memOperations.IsOverlapped( access, i->memAccess ) ){
            return &(*i);
        }
    }
    return NULL;
}

// Calculate a next PC from the executed state of an op.
PC ForwardEmulator::GetExecutedNextPC( PC current, OpStateIF* state ) const
{
    if( state->GetTaken() ){
        return state->GetTakenPC();
    }
    else{
        current.address += SimISAInfo::INSTRUCTION_WORD_BYTE_SIZE;
        return current;
    }
}

// Update fixed pc/retirement id with execution results.
void ForwardEmulator::UpdateFixedPath( OpIterator simOp )
{
    int tid = simOp->GetTID();
    ThreadContext* thread = &m_threadContext[ tid ];
    u64 rid = simOp->GetRetireID();

    if( ( thread->nextFixedRetireID == rid && thread->nextFixedPC == simOp->GetPC() ) ||
        thread->nextFixedRetireID > rid 
    ){
        // When re-fetch recovery occurs (nextFixedRetireID > rid ),
        // nextFixedRetireID and nextFixedPC must be set to a recovered ones. 

        // 'reverse' is set to false, because it may be found near the head of 'inflightOps'.
        InflightOp* inflightOp = GetInflightOp( simOp ); 
        ASSERT( inflightOp != NULL );

        if( inflightOp->updatePC ){
            thread->nextFixedPC = 
                GetExecutedNextPC( inflightOp->emuOp.GetPC(), &inflightOp->emuOp );
        }
        else
        {
            // You must overwrite nextFixedPC because
            // it might hold illegal PC as a result of recovery.
            thread->nextFixedPC = inflightOp->emuOp.GetPC();
        }
        thread->nextFixedRetireID = rid + 1;
    }

    ASSERT(
        ((s64)rid - (s64)thread->nextFixedRetireID) < MAX_RID_DIFFERENCE, 
        "The difference between a current retirement id and a next fixed retirement id is too large."
    );

}
