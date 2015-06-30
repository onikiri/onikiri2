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


#ifndef SIM_FORWARD_EMULATOR_H
#define SIM_FORWARD_EMULATOR_H

#include "Types.h"
#include "Interface/MemIF.h"
#include "Interface/MemAccess.h"

#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/System/ArchitectureState.h"
#include "Sim/System/EmulationSystem/EmulationOp.h"
#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Memory/MemOrderManager/MemOrderOperations.h"

namespace Onikiri 
{
    class Thread;
    
    // An emulator that executes ops on instruction fetch.
    // Execution is done in-order and results are used for validation.
    class ForwardEmulator :
        public PhysicalResourceNode,
        public MemIF
    {
    public:
        ForwardEmulator();
        virtual ~ForwardEmulator();

        void Initialize( InitPhase phase );
        void Finalize();
        void SetContext( const ArchitectureStateList& context );

        void OnFetch( OpIterator op );
        void OnBranchPrediction( PC* predPC );
        void OnCommit( OpIterator op );

        // Get a pre-executed result corresponding to 'op'.
        const OpStateIF* GetExecutionResult( OpIterator op );
        const MemAccess* GetMemoryAccessResult( OpIterator op );

        // Returns whether 'op' is in miss predicted path or not.
        bool IsInMissPredictedPath( OpIterator op );

        //
        // MemIF
        //
        virtual void Read( MemAccess* access );
        virtual void Write( MemAccess* access );

        // Accessors
        EmulatorIF* GetEmulator() const { return m_emulator; };
        bool IsEnabled() const { return m_enable; }

        //
        // PhysicalResouceNode
        //
        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY( "@Enable", m_enable );
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
            END_PARAM_PATH()
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( EmulatorIF, "emulator", m_emulator )
            RESOURCE_ENTRY( Thread, "thread",   m_threads )
        END_RESOURCE_MAP()



    protected:

        PhysicalResourceArray<Thread> m_threads;
        EmulatorIF* m_emulator;
        ArchitectureStateList m_context;
        MemOrderOperations m_memOperations;
        
        // Additional contexts for a forward emulator.
        struct ThreadContext
        {
            // Branch prediction is correctly done to these retirement id/PC.
            // These are used for detecting whether an op is in miss predicted path or not.
            u64 nextFixedRetireID;
            PC  nextFixedPC;

            ThreadContext() :
                nextFixedRetireID(0)
            {
            }
        };

        struct InflightOp
        {
            EmulationOp emuOp;
            OpIterator  simOp;
            MemAccess   memAccess;
            u64         retireId;
            bool        updatePC;
            bool        updateMicroOpIndex;
            bool        isStore;

            InflightOp() : 
                retireId( 0 ),
                updatePC( 0 ),
                updateMicroOpIndex( 0 ),
                isStore( false )
            {
            }
        };

        typedef pool_list< InflightOp > InflightOpList;
        std::vector< InflightOpList >   m_inflightOps;
        std::vector< ThreadContext >    m_threadContext;

        bool m_enable;
        static const s64 MAX_RID_DIFFERENCE = 1024;

        // Update architecture state.
        void UpdateArchContext( 
            ArchitectureState* context, 
            OpStateIF* state, 
            OpInfo* info, 
            bool updatePC, 
            bool updateMicroOpIndex
        );

        // When 'reverse' is true, an in-flight op is searched from the back of 'inflightOps'.
        InflightOp* GetInflightOp( OpIterator op, bool reverse = true );

        InflightOp* GetProducerStore( const InflightOp& consumerLoad );

        // Calculate a next PC from the executed state of an op.
        PC GetExecutedNextPC( PC current, OpStateIF* state ) const;

        // Update fixed pc/retirement id with execution results.
        void UpdateFixedPath( OpIterator simOp );
    };

}; // namespace Onikiri

#endif // SIM_FORWARD_EMULATOR_H

