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

#include "Sim/Predictor/DepPred/MemDepPred/PerfectMemDepPred.h"
#include "Sim/Op/Op.h"
#include "Sim/System/ForwardEmulator.h"
#include "Sim/InorderList/InorderList.h"
#include "Sim/Core/Core.h"

using namespace Onikiri;



PerfectMemDepPred::PerfectMemDepPred() :
    m_fwdEmulator( NULL ),
    m_inorderList( NULL )
{

}

PerfectMemDepPred::~PerfectMemDepPred()
{
    ReleaseParam();
}

void PerfectMemDepPred::Initialize( InitPhase phase )
{
    // Do not call "Initialize()" of a proxy target object,
    // because the "Initialize()" is called by the resource system.

    if( phase == INIT_PRE_CONNECTION ){
        LoadParam();
    }
    else if( phase == INIT_POST_CONNECTION ){
        CheckNodeInitialized( "forwardEmulator", m_fwdEmulator );
        CheckNodeInitialized( "inorderList", m_inorderList );

        // Set endian information to the memory operation object.
        ISAInfoIF* isa = m_fwdEmulator->GetEmulator()->GetISAInfo();
        m_memOperations.SetTargetEndian( isa->IsLittleEndian() );
        m_memOperations.SetAlignment( isa->GetMaxMemoryAccessByteSize() );

        if( !m_fwdEmulator->IsEnabled() ){
            THROW_RUNTIME_ERROR(
                "A perfect memory dependency predictor requires that a forawrd emulator is enabled." 
            );
        }
    }
}

void PerfectMemDepPred::Resolve( OpIterator op )
{
    const OpClass& opClass = op->GetOpClass();
    
    if( !opClass.IsMem() ){
        // Not loads/stores ops do not have memory dependencies.
        return;
    }

    if( m_fwdEmulator->IsInMissPredictedPath( op ) ){
        if( opClass.IsLoad() ){
            // If an op is in a mis-predicted path,
            // a dummy dependency that is never satisfied is set.
            MemDependencyPtr tmpMem(
                m_memDepPool.construct( op->GetCore()->GetNumScheduler() ) 
            );
            tmpMem->Clear();
            op->SetSrcMem( 0, tmpMem );
        }
        // It is no problem that sores are executed.
        return;
    }
    

    // Check memory addresses executed in a forward emulator, and
    // set a dependency to an actually dependent op.

    const MemAccess* consumer = m_fwdEmulator->GetMemoryAccessResult( op );
    //if( !consumer )   return;
    ASSERT( consumer != NULL, "Could not get a pre-executed memory access result. %s", op->ToString(6).c_str() );

    bool consumerIsStore = opClass.IsStore();

    OpIterator i = m_inorderList->GetPrevIndexOp( op );

    // Search a dependent store from 'op'.
    while( !i.IsNull() ){

        if( !i->GetOpClass().IsStore() ){
            i = m_inorderList->GetPrevIndexOp( i );
            continue;
        }

        const MemAccess* producer = m_fwdEmulator->GetMemoryAccessResult( i );
        //if( !producer ) return;
        ASSERT( producer != NULL, "Could not get a pre-executed memory access result. %s", op->ToString(6).c_str() );

        
        if( consumerIsStore ){ 
            // A store is dependent to a store with the same aligned address,
            // because the predictor cannot determine which store is a producer
            // of a load when partial read occurs.
            if( m_memOperations.IsOverlappedInAligned( *consumer, *producer ) ){
                op->SetSrcMem( 0, i->GetDstMem(0) );
                break;
            }
        }
        else{
            if( m_memOperations.IsOverlapped( *consumer, *producer ) ){
                op->SetSrcMem( 0, i->GetDstMem(0) );
                break;
            }
        }

        i = m_inorderList->GetPrevIndexOp( i );
    }
}

void PerfectMemDepPred::Allocate( OpIterator op )
{
    if( !op->GetOpClass().IsMem() ) {
        return;
    }

    // Allocate and assign a memory dependency.
    if( op->GetDstMem(0) == NULL ) {
        MemDependencyPtr tmpMem(
            m_memDepPool.construct( op->GetCore()->GetNumScheduler() ) 
        );
        tmpMem->Clear();
        op->SetDstMem( 0, tmpMem );
    }
}

void PerfectMemDepPred::Commit( OpIterator op )
{
    // Do nothing.
}

void PerfectMemDepPred::Flush( OpIterator op )
{
    // Do nothing.
}

void PerfectMemDepPred::OrderConflicted( OpIterator producer, OpIterator consumer )
{
    ASSERT( 
        false, 
        "A perfect memory dependency predictor must not predict incorrect dependencies. \n\n"
        "Conflicted producer:\n %s\n\nConflicted consumer:\n%s\n",
        producer->ToString(6).c_str(),
        consumer->ToString(6).c_str()
    );
}

bool PerfectMemDepPred::CanAllocate( OpIterator* infoArray, int numOp )
{
    return true;
}
