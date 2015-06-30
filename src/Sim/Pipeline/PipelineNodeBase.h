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


#ifndef SIM_PIPELINE_PIPELINE_NODE_BASE_H
#define SIM_PIPELINE_PIPELINE_NODE_BASE_H

#include "Utility/RuntimeError.h"

#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/Pipeline/PipelineNodeIF.h"
#include "Sim/Pipeline/Pipeline.h"
#include "Sim/Pipeline/PipelineLatch.h"

namespace Onikiri 
{
    class Thread;
    class Core;

    // A base class of Fetcher/Renamer/Dispatcher/Scheduler/Retirer
    class PipelineNodeBase :
        public ClockedResourceBase,
        public PipelineNodeIF,
        public PhysicalResourceNode
    {

    public:
        
        const char* Who() const 
        {
            return PhysicalResourceNode::Who(); 
        };

        PipelineNodeBase() : 
            m_initialized( false ),
            m_enableLatch( true ),
            m_upperPipelineNode( NULL ),
            m_upperPipeline( NULL ),
            m_lowerPipelineNode( NULL )
        {
            BaseT::AddChild( &m_latch );            // gcc needs 'BaseT::'
            BaseT::AddChild( &m_lowerPipeline );    
            m_lowerPipeline.AddUpperPipelineNode( this );
        }

        virtual ~PipelineNodeBase()
        {
            ASSERT( m_initialized, "Initialize() has not been called." );
        }

        virtual void Initialize( InitPhase phase )
        {
            if( phase == INIT_POST_CONNECTION ){
                if( m_thread.GetSize() == 0 ) {
                    THROW_RUNTIME_ERROR("thread not set.");
                }
                if( m_core.GetSize() == 0 ) {
                    THROW_RUNTIME_ERROR("core not set.");
                }
            }

            m_initialized = true;
        }

        // Stall handlers
        virtual void StallThisCycle()
        {
            BaseT::StallThisCycle();
            
            PipelineNodeIF* upper = GetUpperPipelineNode();
            if( upper ){
                upper->StallThisCycle();
            }
        }

        virtual void StallNextCycle( int cycle )
        {
            BaseT::StallNextCycle( cycle );

            PipelineNodeIF* upper = GetUpperPipelineNode();
            if( upper ){
                upper->StallNextCycle( cycle );
            }
        }

        // Stall only upper pipelines and the latch.
        // A pipeline next to this node is not stalled.
        virtual void StallThisNodeAndUpperThisCycle()
        {
            PipelineNodeIF* upper = GetUpperPipelineNode();
            if( upper ){
                upper->StallThisCycle();
            }
            if( m_enableLatch ){
                m_latch.StallThisCycle();
            }
            BaseT::StallThisCycleExcludingChildren();   // gcc needs 'BaseT::'
        }

        // This method is called when an op exits on upper pipeline.
        // A pipeline that writes a buffer like Dispatcher
        // overwrites this method.
        virtual void ExitUpperPipeline( OpIterator op )
        {
            if( m_enableLatch ){
                m_latch.Receive( op );
            }
        };
        
        // This method is called when an op exits an lower pipeline.
        virtual void ExitLowerPipeline( OpIterator op )
        {
        };

        // Link pipeline nodes.
        virtual void SetUpperPipelineNode( PipelineNodeIF* upper )
        {
            m_upperPipelineNode = upper;
            m_upperPipeline     = upper->GetLowerPipeline();
            upper->SetLowerPipelineNode( this );
        }

        virtual void SetLowerPipelineNode( PipelineNodeIF* lower )
        {
            m_lowerPipelineNode = lower;
        }

        // 下流のPipelineを取得
        virtual Pipeline* GetLowerPipeline()
        {
            return &m_lowerPipeline;
        }

        // 下流のPipelineNodeを取得
        virtual PipelineNodeIF* GetLowerPipelineNode()
        {
            return m_lowerPipelineNode;
        }

        // 上流のパイプラインノードを取得
        virtual PipelineNodeIF* GetUpperPipelineNode()
        {
            return m_upperPipelineNode;
        }

        // Returns whether this node can allocate entries or not.
        virtual bool CanAllocate( int ops )
        {
            return true;
        }

        // Add an external lower pipeline.
        virtual void AddLowerPipeline( Pipeline* pipe )
        {
            this->AddChild( pipe ); // gcc needs 'BaseT::'
            pipe->AddUpperPipelineNode( this );
            m_exLowerPipelines.push_back( pipe );
        }

        virtual void Commit( OpIterator op ){}
        virtual void Cancel( OpIterator op ){}

        virtual void Flush( OpIterator op )
        {
            if( m_enableLatch ){
                m_latch.Delete( op );
            }
            m_lowerPipeline.Flush( op );
            for( size_t i = 0; i < m_exLowerPipelines.size(); i++ ){
                m_exLowerPipelines[i]->Flush( op );
            }
        }

        virtual void Retire( OpIterator op )
        {
            if( m_enableLatch ){
                m_latch.Delete( op );
            }
            m_lowerPipeline.Retire( op );
            for( size_t i = 0; i < m_exLowerPipelines.size(); i++ ){
                m_exLowerPipelines[i]->Retire( op );
            }
        }

        // Disable a pipeline latch if a latch is not used for optimization.
        void DisableLatch()
        {
            for( Children::iterator i = m_children.begin(); i != m_children.end(); ++i ){
                if( *i == (ClockedResourceIF*)&m_latch ){
                    m_children.erase( i );
                    break;
                }
            }
            m_enableLatch = false;
        }

        // accessors
        Core* GetCore( int index = 0 ) { return m_core[index]; }
    protected:
        bool m_initialized;
        bool m_enableLatch;
        typedef ClockedResourceBase BaseT;

        PipelineLatch m_latch;
        PhysicalResourceArray<Thread> m_thread;
        PhysicalResourceArray<Core>   m_core;

        PipelineNodeIF* m_upperPipelineNode;
        Pipeline*       m_upperPipeline;
        PipelineNodeIF* m_lowerPipelineNode;
        Pipeline        m_lowerPipeline;
        std::vector<Pipeline*>  m_exLowerPipelines;
    };

}; // namespace Onikiri

#endif // SIM_PIPELINE_PIPELINE_NODE_BASE_H
