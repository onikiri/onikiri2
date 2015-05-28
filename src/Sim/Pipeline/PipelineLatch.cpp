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

#include "Sim/Pipeline/PipelineLatch.h"
#include "Sim/Dumper/Dumper.h"

using namespace std;
using namespace Onikiri;

bool PipelineLatch::FindAndEraseFromLatch( List* latch, OpIterator op )
{
    iterator target = 
        std::find( latch->begin(), latch->end(), op );

    if( target != latch->end() ){
        latch->erase( target );
        return true;
    }
    else{
        return false;
    }
}
        
void PipelineLatch::DumpStallBegin()
{
    if( m_enableDumpStall && !m_latchOutIsStalled ){
        for( iterator i = m_latchOut.begin(); i != m_latchOut.end(); ++i ){
            g_dumper.DumpStallBegin( *i );
        }
        m_latchOutIsStalled = true;
    }
}

void PipelineLatch::DumpStallEnd()
{
    if( m_enableDumpStall && m_latchOutIsStalled ){
        for( iterator i = m_latchOut.begin(); i != m_latchOut.end(); ++i ){
            g_dumper.DumpStallEnd( *i );
        }
        m_latchOutIsStalled = false;
    }
}

// Update a latch
void PipelineLatch::UpdateLatch()
{
    typedef List::iterator iterator;
    for( iterator i = m_latchIn.begin(); i != m_latchIn.end(); ){
        m_latchOut.push_back( *i );
        i = m_latchIn.erase( i );
    }
}


PipelineLatch::PipelineLatch( const char* name ) : 
    BaseType( name ),
    m_enableDumpStall( true ),
    m_latchOutIsStalled( false )
{
}

void PipelineLatch::Receive( OpIterator op )
{
    ASSERT(
        !IsStalledThisCycle(),
        "The latch is written on a stalled cycle."
    );
    m_latchIn.push_back( op );
}

// A cycle end handler
// Update the pipeline latch.
void PipelineLatch::End()
{
    ASSERT( 
        !( !IsStalledThisCycle() && m_latchOut.size() > 0 ), 
        "Outputs are not picked on an un-stalled cycle."
    );

    ASSERT( 
        !( m_latchIn.size() > 0 && m_latchOut.size() > 0 ), 
        "Data on the pipeline latch is overwritten by a upper pipeline. "
        "The data has not processed for pipeline stall."
    );

    UpdateLatch();
    BaseType::End();
}

void PipelineLatch::Transition()
{
    BaseType::Transition();
    if( IsStalledThisCycle() ){
        // BeginStall is only called 
        DumpStallBegin();
    }
}

// Stall handlers, which are called in stall begin/end
void PipelineLatch::BeginStall()
{
    BaseType::BeginStall();
    DumpStallBegin();
}

void PipelineLatch::EndStall()
{
    BaseType::EndStall();
    DumpStallEnd();
};
        
void PipelineLatch::Delete( OpIterator op )
{
    if( FindAndEraseFromLatch( &m_latchIn, op ) ){
        return;
    }
    if( FindAndEraseFromLatch( &m_latchOut, op ) ){
        return;
    }
}

void PipelineLatch::EnableDumpStall( bool enable )
{
    m_enableDumpStall = enable;
}
