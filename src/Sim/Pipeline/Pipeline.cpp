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

#include "Sim/Pipeline/Pipeline.h"
#include "Sim/Pipeline/PipelineNodeIF.h"
#include "Sim/Op/Op.h"
#include "Sim/Dumper/Dumper.h"

using namespace std;
using namespace Onikiri;

//
// An event writing an op to a pipeline latch.
//
class OpPipelineProcessEndEvent :
    public EventBase<OpPipelineProcessEndEvent>
{
protected:
    OpIterator m_op;
    Pipeline*  m_pipe;
    PipelineNodeIF* m_lowerNode;

public:
    OpPipelineProcessEndEvent( OpIterator op, Pipeline* pipe, PipelineNodeIF* lowerNode ) : 
        m_op  ( op ),
        m_pipe( pipe ),
        m_lowerNode( lowerNode )
    {
    }

    virtual void Update()
    {
        m_pipe->ExitPipeline( m_op, m_lowerNode );
    }
};


Pipeline::Pipeline() :
    m_enableDumpStall(true)
{
}

Pipeline::~Pipeline()
{
}


void Pipeline::EnterPipeline( OpIterator op, int time, PipelineNodeIF* lowerNode )
{
    m_opList.push_back( op );
    EventPtr evnt( OpPipelineProcessEndEvent::Construct( op, this, lowerNode ) );
    if( time == 0 ){
        evnt->Update();
    }
    else{
        op->AddEvent( evnt, this, time );
    }
}

void Pipeline::ExitPipeline( OpIterator op, PipelineNodeIF* lowerNode )
{
    lowerNode->ExitUpperPipeline( op );
    for( size_t i = 0; i < m_upperPipelineNodes.size(); i++ ){
        m_upperPipelineNodes[i]->ExitLowerPipeline( op );
    }
    m_opList.remove( op );
}

void Pipeline::AddUpperPipelineNode( PipelineNodeIF* node )
{
    m_upperPipelineNodes.push_back( node );
}

int Pipeline::GetOpCount()
{
    return (int)m_opList.size();
}

void Pipeline::Retire( OpIterator op )
{
    m_opList.remove( op );
}

void Pipeline::Flush( OpIterator op )
{
    m_opList.remove( op );
}

void Pipeline::BeginStall()
{
    if( !g_dumper.IsEnabled() || !m_enableDumpStall )
        return;

    for( List::iterator i = m_opList.begin(); i != m_opList.end(); ++i ){
        g_dumper.DumpStallBegin( *i );
    }
}

void Pipeline::EndStall()
{
    if( !g_dumper.IsEnabled() || !m_enableDumpStall )
        return;

    for( List::iterator i = m_opList.begin(); i != m_opList.end(); ++i ){
        g_dumper.DumpStallEnd( *i );
    }
}


void Pipeline::EnableDumpStall( bool enable )
{
    m_enableDumpStall = enable;
}
