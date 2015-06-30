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


#ifndef SIM_CORE_DATA_PRED_TYPES_H
#define SIM_CORE_DATA_PRED_TYPES_H

// Definition of a type, policy and point of 
// data prediction miss recovery.
// Actual implementation of data prediction miss recovery is in
// Core, InorderList, MemOrderManager...

#include "Env/Param/ParamExchange.h"

namespace Onikiri 
{

    class DataPredMissRecovery : public ParamExchangeChild
    {
    public:

        //
        // Data prediction type
        //
        enum Type 
        {
            TYPE_LATENCY,       // Latency prediction

            TYPE_ADDRESS_MATCH, // Address match/unmatch prediction

            TYPE_PARTIAL_LOAD,  // Partial load
                                // This type is not exactly data prediction, 
                                // but this is defined here.
                                // This type defines a recovery method on partial load,
                                // which read data that is written by its predecessor.
            
            TYPE_VALUE,         // Value prediction
                                // Value prediction is not implemented yet and this
                                // is for user extension.

            TYPE_END            // End sentinel
        };

        //
        // Recovery policy
        //
        enum Policy 
        {
            // Re-fetch
            POLICY_REFETCH,         
            
            // Invalidate ready signals of ALL successor instructions 
            // and re-issue these from a issue queue.
            // This policy requires that a scheduler must
            // hold instructions after instruction issue.
            POLICY_REISSUE_ALL,     

            // Invalidate ONLY ready signals of dependent successor instructions 
            // and re-issue these from a issue queue.
            // This policy requires that a scheduler must
            // hold instructions after instruction issue.
            POLICY_REISSUE_SELECTIVE,   
        
            // Invalidate ready signals of ALL Unfinished instructions
            // and re-issue these from a issue queue.
            // This policy requires that a scheduler 
            // hold instructions before instruction finish.
            POLICY_REISSUE_NOT_FINISHED,

            // Re-dispatch ALL successor instructions from a re-dispatch buffer.
            POLICY_REDISPATCH_ALL,

            // End sentinel
            POLICY_END
        };

        // A point from which instructions are recovered
        enum From 
        {
            // Recovered from a producer( an instruction that did prediction ).
            // This means that an instruction that did prediction itself is recovered.
            FROM_PRODUCER,

            // Recovered from a next instruction of a producer.
            FROM_NEXT_OF_PRODUCER,

            // Recovered from a first consumer instruction of a producer.
            FROM_CONSUMER,
            
            // End sentinel
            FROM_END
        };

        BEGIN_PARAM_MAP( "" )

            BEGIN_PARAM_BINDING(  "@Type", m_type, Type )
                PARAM_BINDING_ENTRY( "Latency",         TYPE_LATENCY )
                PARAM_BINDING_ENTRY( "AddressMatch",    TYPE_ADDRESS_MATCH )
                PARAM_BINDING_ENTRY( "PartialLoad",     TYPE_PARTIAL_LOAD )
                PARAM_BINDING_ENTRY( "Value",           TYPE_VALUE )
            END_PARAM_BINDING()

            BEGIN_PARAM_BINDING(  "@Policy", m_policy, Policy )
                PARAM_BINDING_ENTRY( "Refetch",     POLICY_REFETCH )
                PARAM_BINDING_ENTRY( "ReissueAll",  POLICY_REISSUE_ALL )
                PARAM_BINDING_ENTRY( "ReissueSelective", POLICY_REISSUE_SELECTIVE )
                PARAM_BINDING_ENTRY( "ReissueNotFinished", POLICY_REISSUE_NOT_FINISHED )
                PARAM_BINDING_ENTRY( "Redispatch",  POLICY_REDISPATCH_ALL )
            END_PARAM_BINDING()

            BEGIN_PARAM_BINDING(  "@From", m_from, From )
                PARAM_BINDING_ENTRY( "Producer",        FROM_PRODUCER )
                PARAM_BINDING_ENTRY( "NextOfProducer",  FROM_NEXT_OF_PRODUCER )
                PARAM_BINDING_ENTRY( "Consumer",        FROM_CONSUMER )
            END_PARAM_BINDING()

        END_PARAM_MAP()

        DataPredMissRecovery( Type type = TYPE_END, Policy policy = POLICY_END, From from = FROM_END ) : 
            m_type  ( type ),
            m_policy( policy ), 
            m_from  ( from   )
        {}

        Policy GetPolicy()  const { return m_policy;    }
        From     GetFrom()  const { return m_from;      }

        bool IsRefetch()            const   {   return m_policy == POLICY_REFETCH;          }
        bool IsReissueAll()         const   {   return m_policy == POLICY_REISSUE_ALL;      }
        bool IsReissueSelective()   const   {   return m_policy == POLICY_REISSUE_SELECTIVE;}
        bool IsRedispatch()         const   {   return m_policy == POLICY_REDISPATCH_ALL;   }
        bool IsReissueNotFinished() const   {   return m_policy == POLICY_REISSUE_NOT_FINISHED;         }

        bool FromProducer()         const { return m_from == FROM_PRODUCER;         }
        bool FromNextOfProducer()   const { return m_from == FROM_NEXT_OF_PRODUCER; }
        bool FromConsumer()         const { return m_from == FROM_CONSUMER;         }


        // Check integrity of 'policy' and 'from'.
        void Validate();

        // Decide whether checkpointing is required or not for an op of 'opClass'.
        bool IsRequiredBeforeCheckpoint( const OpClass& opClass ) const;
        bool IsRequiredAfterCheckpoint ( const OpClass& opClass ) const;

    protected:

        Type    m_type;
        Policy  m_policy;
        From    m_from;
    };


    // A pipeline model of a load instruction.
    enum LoadPipelineModel
    {
        LPM_INVALID,

        // Add ports to a RF for a higher level cache read
        LPM_SINGLE_ISSUE,   

        // Do not add ports to a RF and issue load instruction
        // more than once.                                  
        LPM_MULTI_ISSUE     
    };

    // Whether forget or retain ops in a scheduler after issue.
    enum SchedulerRemovePolicy
    {
        RP_FOLLOW_CORE,         // Follow a policy of a core.
        RP_REMOVE,              // Remove ops after issue.
        RP_RETAIN,              // Retain ops to commit. (Remove ops after commit.)
        RP_REMOVE_AFTER_FINISH  // Remove ops after op finishes.
                                // This policy is similar to that of Alpha 21264.
    };

    // Where are checkpoints taken.
    enum CheckpointingPolicy
    {
        // Checkpoints are taken for all ops.
        CP_ALL, 

        // Checkpoints are taken for necessary ops the require 
        // Core::IsRequiredCheckpointBefore() and Core::IsRequiredCheckpointAfter.
        CP_AUTO 
    };
}; // namespace Onikiri

#endif // SIM_CORE_DATA_PRED_TYPES_H

