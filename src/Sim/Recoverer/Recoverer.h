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


#ifndef SIM_RECOVERER_RECOVERER_H
#define SIM_RECOVERER_RECOVERER_H

#include "Sim/Foundation/Hook/HookDecl.h"
#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Op/OpContainer/OpList.h"
#include "Sim/ExecUnit/ExecUnitIF.h"
#include "Sim/Core/DataPredTypes.h"

namespace Onikiri
{
    class InorderList;
    class Core;
    class Thread;
    class Checkpoint;
    class CheckpointMaster;

    class Recoverer : public PhysicalResourceNode
    {
    public:
        typedef DataPredMissRecovery Recovery;

        // parameter mapping
        BEGIN_PARAM_MAP("")
            
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY( "@BranchPredRecoveryLatency",  m_brPredRecoveryLatency )
                PARAM_ENTRY( "@ExceptionRecoverylatency",   m_exceptionRecoveryLatency )
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
                BEGIN_PARAM_PATH( "LatPredRecovery/" )
                    PARAM_ENTRY( "@NumRecovery",    m_latPredRecoveryCount )
                    PARAM_ENTRY( "@NumOps",         m_latPredRecoveryOps )
                END_PARAM_PATH()
                BEGIN_PARAM_PATH( "AddrPredRecovery/" )
                    PARAM_ENTRY( "@NumRecovery",    m_addrPredRecoveryCount )
                    PARAM_ENTRY( "@NumOps",         m_addrPredRecoveryOps )
                END_PARAM_PATH()
                BEGIN_PARAM_PATH( "ValuePredRecovery/" )
                    PARAM_ENTRY( "@NumRecovery",    m_valuePredRecoveryCount )
                    PARAM_ENTRY( "@NumOps",         m_valuePredRecoveryOps )
                END_PARAM_PATH()
                BEGIN_PARAM_PATH( "PartialLoadRecovery/" )
                    PARAM_ENTRY( "@NumRecovery",    m_partialReadRecoveryCount )
                    PARAM_ENTRY( "@NumOps",         m_partialReadRecoveryOps )
                END_PARAM_PATH()
                BEGIN_PARAM_PATH( "BracnchPredRecovery/" )
                    PARAM_ENTRY( "@NumRecovery",    m_brPredRecoveryCount )
                    PARAM_ENTRY( "@NumOps",         m_brPredRecoveryOps )
                END_PARAM_PATH()
                BEGIN_PARAM_PATH( "ExceptionRecovery/" )
                    PARAM_ENTRY( "@NumRecovery",    m_exceptionRecoveryCount )
                    PARAM_ENTRY( "@NumOps",         m_exceptionRecoveryOps )
                END_PARAM_PATH()
            END_PARAM_PATH()

        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( Core,               "core",             m_core )
            RESOURCE_ENTRY( Thread,             "thread",           m_thread )
            RESOURCE_ENTRY( InorderList,        "inorderList",      m_inorderList )
            RESOURCE_ENTRY( CheckpointMaster,   "checkpointMaster", m_checkpointMaster )
        END_RESOURCE_MAP()

        Recoverer();
        virtual ~Recoverer();

        virtual void Initialize( InitPhase phase );
        virtual void Finalize();

        
        // Recover a processor from branch miss prediction.
        // Ops after 'branch' are flushed and re-fetched.
        void RecoverBPredMiss( OpIterator branch );

        // Recover a processor from exception.
        void RecoverException( OpIterator causer );

        // Recover a processor from data miss prediction.
        // These methods return the number of squashed/canceled instructions.
        int RecoverDataPredMiss( 
            OpIterator producer,
            OpIterator consumer, 
            DataPredMissRecovery::Type dataPredType 
        );


        // Recover a processor from general miss prediction.
        int RecoverDataPredMiss( 
            OpIterator producer,
            OpIterator consumer, 
            const DataPredMissRecovery& recovery
        );
                
        // Re-fetch all ops.
        int RecoverByRefetch( OpIterator missedOp, OpIterator startOp );

        // Re-issue all ops.
        int RecoverByRescheduleAll( OpIterator missedOp, OpIterator startOp );

        // Re-issue not finished ops.
        int RecoverByRescheduleNotFinished(OpIterator missedOp, OpIterator startOp);

        // Re-issue consumer ops selectively.
        int RecoverByRescheduleSelective( OpIterator producerOp, Recovery::From from );


    protected:

        Core*               m_core;
        Thread*             m_thread;
        InorderList*        m_inorderList;
        CheckpointMaster*   m_checkpointMaster;

        s64 m_latPredRecoveryCount;
        s64 m_addrPredRecoveryCount;
        s64 m_valuePredRecoveryCount;
        s64 m_partialReadRecoveryCount;

        s64 m_latPredRecoveryOps;
        s64 m_addrPredRecoveryOps;
        s64 m_valuePredRecoveryOps;
        s64 m_partialReadRecoveryOps;

        s64 m_brPredRecoveryCount;
        s64 m_brPredRecoveryOps;

        s64 m_exceptionRecoveryCount;
        s64 m_exceptionRecoveryOps;

        int m_brPredRecoveryLatency;
        int m_exceptionRecoveryLatency;

        // Recover the state of the processor to 'checkpoint'.
        void RecoverCheckpoint( Checkpoint* checkpoint );

        // Update recovery statistics
        void UpdateRecoveryStatistics(
            int recoveredInsns, 
            Recovery::Type dataPredType 
        );

        // Re-schedule the consumers of a producer.
        // This is for RecoverByRescheduleSelective().
        int RescheduleConsumers( OpIterator producer );

        // Get an op that starts recovery.
        OpIterator GetRecoveryStartOp( OpIterator producer, OpIterator consumer, Recovery::From from );
    };
}

#endif

