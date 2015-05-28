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

#include "Recoverer.h"

#include "Sim/Foundation/Hook/HookUtil.h"
#include "Sim/Foundation/Checkpoint/CheckpointMaster.h"
#include "Sim/Foundation/Checkpoint/CheckpointedData.h"

#include "Sim/Op/Op.h"
#include "Sim/Core/Core.h"
#include "Sim/Thread/Thread.h"
#include "Sim/InorderList/InorderList.h"
#include "Sim/Memory/MemOrderManager/MemOrderManager.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/Pipeline/Fetcher/Fetcher.h"


using namespace Onikiri;

Recoverer::Recoverer() : 
    m_core(0),
    m_thread(0),
    m_inorderList(0),
    m_checkpointMaster(0),
    m_latPredRecoveryCount(0),
    m_addrPredRecoveryCount(0),
    m_valuePredRecoveryCount(0),
    m_partialReadRecoveryCount(0),
    m_latPredRecoveryOps(0),
    m_addrPredRecoveryOps(0),
    m_valuePredRecoveryOps(0),
    m_partialReadRecoveryOps(0),
    m_brPredRecoveryCount(0),
    m_brPredRecoveryOps(0),
    m_exceptionRecoveryCount(0),
    m_exceptionRecoveryOps(0),
    m_brPredRecoveryLatency(0),
    m_exceptionRecoveryLatency(0)
{
}

Recoverer::~Recoverer()
{
}

void Recoverer::Initialize( InitPhase phase )
{
    if( phase == INIT_PRE_CONNECTION ){
        LoadParam();
    }
    else if( phase == INIT_POST_CONNECTION ){

        // Policy checking.
        // Other policy checking is done in Scheduler.
        const DataPredMissRecovery& pmr = m_core->GetPartialLoadRecovery();
        if( pmr.FromNextOfProducer() || pmr.FromProducer() ){
            // Partial load violation may be detected after a producer is committed.
            THROW_RUNTIME_ERROR( "Partial load recovery supports only a mode 'from Consumer'." );
        }
    }
}

void Recoverer::Finalize()
{
    ReleaseParam();
}

// Recover a processor from miss branch prediction.
// Ops after 'branch' are flushed and re-fetched.
void Recoverer::RecoverBPredMiss( OpIterator branch )
{
    // Recover processor state to a checkpoint after the branch.
    RecoverCheckpoint( branch->GetAfterCheckpoint() );

    // Flush backward ops.
    m_brPredRecoveryOps +=
        m_inorderList->FlushBackward( m_inorderList->GetNextPCOp(branch) );

    // Set a correct branch result.
    m_thread->SetFetchPC( branch->GetNextPC() );

    // Stall a fetcher for a recovery latency.
    int latency = m_brPredRecoveryLatency;
    if( latency > 0 ){
        m_thread->GetCore()->GetFetcher()->StallNextCycle( latency );
    }

    m_brPredRecoveryCount++;
}

// Recover a processor from exception.
void Recoverer::RecoverException( OpIterator causer )
{
    ASSERT(
        !causer->GetOpClass().IsSyscall(), 
        "Exception of a system call is not supported."
    );

    m_exceptionRecoveryOps += 
        RecoverByRefetch( causer, causer );

    // Clear an exception state
    Exception exception = causer->GetException();
    exception.exception = false;
    causer->SetException( exception );

    // Stall a fetcher for a recovery latency.
    int latency = m_exceptionRecoveryLatency;
    if( latency > 0 ){
        m_thread->GetCore()->GetFetcher()->StallNextCycle( latency );
    }

    m_exceptionRecoveryCount++;
}

// Recover a processor data miss prediction.
int Recoverer::RecoverDataPredMiss( 
    OpIterator producer, 
    OpIterator consumer, 
    Recovery::Type dataPredType 
){
    // Select a recovery method by a specified type.
    Recovery dpmr;
    typedef Recovery DPMR;

    switch( dataPredType ){
    case DPMR::TYPE_LATENCY:
        dpmr = m_core->GetLatPredMissRecovery();
        // Miss prediction of latency requires reset of dependencies,
        // which are equivalent to ready bits of destinations of 'op'.
        // This is because a producer does not finish when miss prediction 
        // is detected.
        producer->ResetDependency();
        break;

    case DPMR::TYPE_ADDRESS_MATCH:
        dpmr = m_core->GetAddrPredMissRecovery();
        break;

    case DPMR::TYPE_VALUE:
        dpmr = m_core->GetValuePredMissRecovery();
        break;

    case DPMR::TYPE_PARTIAL_LOAD:
        dpmr = m_core->GetPartialLoadRecovery();
        break;

    default:
        THROW_RUNTIME_ERROR( "Unknown data prediction type." );
        break;
    }

    int recoveredInsns = 
        RecoverDataPredMiss( producer, consumer, dpmr );

    UpdateRecoveryStatistics( recoveredInsns, dataPredType );

    return recoveredInsns;
}

int Recoverer::RecoverDataPredMiss( 
    OpIterator producer, OpIterator consumer, const Recovery& dpmr
){
    // Recovered from:
    const Recovery::From& from      = dpmr.GetFrom();

    // Recover policy:
    const Recovery::Policy& policy  = dpmr.GetPolicy();

    if( policy == Recovery::POLICY_REISSUE_SELECTIVE ){
        // In the case of Recovery::FROM_CONSUMER, 
        // multiple consumers must be re-scheduled and 
        // the bellow codes including GetFirstConsumer() cannnot treat this.
        return RecoverByRescheduleSelective( producer, from );  
    }


    // Select a start point of recovery.
    OpIterator startOp = GetRecoveryStartOp( producer, consumer, from );

    // Return if there are no ops recovered...
    if( startOp.IsNull() ) {
        return 0;
    }

    int  recovered = 0;

    switch( policy ){

    case Recovery::POLICY_REFETCH:
        recovered = RecoverByRefetch( producer, startOp );
        break;

    case Recovery::POLICY_REISSUE_ALL:
        recovered = RecoverByRescheduleAll( producer, startOp );
        break;
    
    case Recovery::POLICY_REISSUE_NOT_FINISHED:
        recovered = RecoverByRescheduleNotFinished( producer, startOp );
        break;

    default:
        ASSERT( 0, "Unknown recovery policy." );
        break;
    }

    return recovered;

}


// Re-fetch from 'startOp'.
int Recoverer::RecoverByRefetch( OpIterator /*missedOp*/, OpIterator startOp )
{
    // A pc that fetch is resumed from.
    PC fetchPC = startOp->GetPC();
    
    ASSERT( 
        startOp->GetStatus() < OpStatus::OS_COMITTING,
        "Cannot re-fetch from this op because this op is already committed. op: %s", 
        startOp->ToString(6).c_str()
    );

    // Recover the state to 'checkpoint'.
    Checkpoint* checkpoint = startOp->GetBeforeCheckpoint();
    if( !checkpoint ){
        OpIterator prevOp = m_inorderList->GetPrevPCOp( startOp );
        if( prevOp.IsNull() ){
            THROW_RUNTIME_ERROR( "A necessary checkpoint is not taken. op: %s", startOp->ToString(6).c_str() );
        }
        checkpoint = m_inorderList->GetFrontOpOfSamePC(prevOp)->GetAfterCheckpoint();
        if( !checkpoint ){
            THROW_RUNTIME_ERROR( "A necessary checkpoint is not taken. op: %s", startOp->ToString(6).c_str() );
        }
    }
    RecoverCheckpoint( checkpoint );
    
    // Flush backward ops.
    int flushedInsns = 
        m_inorderList->FlushBackward( m_inorderList->GetFrontOpOfSamePC( startOp ) );

    // Set a correct branch result.
    m_thread->SetFetchPC( fetchPC );

    return flushedInsns;
}


// Re-schedule all ops.
int Recoverer::RecoverByRescheduleAll( OpIterator missedOp, OpIterator startOp )
{
    int recoveredInsns = 0;
    for( OpIterator i = startOp; !i.IsNull(); i = m_inorderList->GetNextIndexOp( i ) ){
        if( i->GetScheduler()->Reschedule( i ) ){
            recoveredInsns++;
        }
    }

    return recoveredInsns;
}

// Reschedule only not finished ops.
int Recoverer::RecoverByRescheduleNotFinished( OpIterator missedOp, OpIterator startOp )
{
    int recoveredInsns = 0;
    for( OpIterator i = startOp; !i.IsNull(); i = m_inorderList->GetNextIndexOp( i ) ){
        if( i->GetStatus() <= OpStatus::OS_EXECUTING ){
            if( i->GetScheduler()->Reschedule( i ) ){
                recoveredInsns++;
            }
        }
    }

    return recoveredInsns;
}

// Selective re-scheduler.
int Recoverer::RecoverByRescheduleSelective( OpIterator producerOp, Recovery::From from )
{
    ASSERT(
        from != Recovery::FROM_NEXT_OF_PRODUCER,
        "cannot reschedule selective next of producer"
    );

    int recoveredInsns = 0;
    if( from == Recovery::FROM_PRODUCER ){
        if( producerOp->GetScheduler()->Reschedule( producerOp ) ){
            recoveredInsns++;
        }
    }
    //recoveredInsns += producerOp->RescheduleConsumers( producerOp );
    RescheduleConsumers( producerOp );

    return recoveredInsns;
}

// checkpoint ‚ÌƒŠƒJƒoƒŠ‚ðs‚¤
void Recoverer::RecoverCheckpoint( Checkpoint* checkpoint )
{
    ASSERT( checkpoint != 0, "no checkpoint for recovery" );
    m_checkpointMaster->Recover( checkpoint );
}

void Recoverer::UpdateRecoveryStatistics( 
    int recoveredInsns, 
    Recovery::Type dataPredType 
){
    typedef Recovery DPMR;

    switch( dataPredType ){

    case DPMR::TYPE_LATENCY:
        m_latPredRecoveryOps += recoveredInsns;
        m_latPredRecoveryCount++;
        break;

    case DPMR::TYPE_ADDRESS_MATCH:
        m_addrPredRecoveryOps += recoveredInsns;
        m_addrPredRecoveryCount++;
        break;

    case DPMR::TYPE_VALUE:
        m_valuePredRecoveryOps += recoveredInsns;
        m_valuePredRecoveryCount++;
        break;

    case DPMR::TYPE_PARTIAL_LOAD:
        m_partialReadRecoveryOps += recoveredInsns;
        m_partialReadRecoveryCount++;
        break;

    default:
        THROW_RUNTIME_ERROR( "Unknown data prediction type." );
        break;

    }
}

// Re-schedule the consumers of a producer.
// This is for RecoverByRescheduleSelective().
int Recoverer::RescheduleConsumers( OpIterator producer )
{
    int reshceduledInsns = 0;

    // Reschedule consumers.
    int depNum = producer->GetDstDepNum();
    for( int i = 0; i < depNum; ++i ){
        Dependency* dep = producer->GetDstDep(i);
        const Dependency::ConsumerListType& consumers = dep->GetConsumers();
        for(Dependency::ConsumerListConstIterator j = consumers.begin();
            j != consumers.end(); 
            ++j
        ){
            OpIterator consumer = *j;
            if( consumer->GetScheduler()->Reschedule( consumer ) ){
                reshceduledInsns++;
            }
            reshceduledInsns += RescheduleConsumers( consumer );
        }
    }
    
    // Rescheduling speculatively executed load instructions.
    if( producer->GetOpClass().IsStore() ) {
        MemOrderManager* memOrder = m_thread->GetMemOrderManager();
        const MemAccess& memAccess = producer->GetMemAccess();
        for( int i = 0; ; i++ ){
            OpIterator consumer = memOrder->GetConsumerLoad( producer, memAccess, i );
            if( consumer.IsNull() )
                break;
            if( consumer->GetScheduler()->Reschedule( consumer ) ){
                reshceduledInsns++;
            }
            reshceduledInsns += RescheduleConsumers( consumer );
        }
    }

    return reshceduledInsns;

}

OpIterator Recoverer::GetRecoveryStartOp( OpIterator producer, OpIterator consumer, Recovery::From from )
{
    // Select a start point of recovery.
    OpIterator startOp(0);
    switch( from ){
    case Recovery::FROM_PRODUCER:
        ASSERT( 
            !producer.IsNull(),
            "The start point of recovery is specified as 'Producer', but a passed producer is null." 
            "This may occur when a producer is already retired when violation is detected."
        );
        startOp = producer;
        break;

    case Recovery::FROM_NEXT_OF_PRODUCER:
        ASSERT( 
            !producer.IsNull(), 
            "The start point of recovery is specified as 'NextOfProducer', but a passed producer is null." 
            "This may occur when a producer is already retired when violation is detected."
        );
        startOp = m_inorderList->GetNextPCOp( producer );
        break;

    case Recovery::FROM_CONSUMER:
        startOp = consumer;
        break;

    default:
        ASSERT( 0, "An unknown recovery start point." );
        break;
    }
    return startOp;
}
