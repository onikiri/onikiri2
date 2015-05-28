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

#include "Sim/Dumper/Dumper.h"

#include "Utility/RuntimeError.h"
#include "Sim/Dumper/TraceDumper.h"
#include "Sim/Dumper/VisualizationDumper.h"
#include "Sim/Dumper/CountDumper.h"
#include "Sim/Op/Op.h"
#include "Sim/Core/Core.h"

namespace Onikiri
{
    Dumper g_dumper;
}

using namespace std;
using namespace Onikiri;

Dumper::Dumper()
{
    m_dumpEnabled = false;
}

Dumper::~Dumper()
{
    ReleaseDumper();
}


void Dumper::CreateThreadDumper( 
    ThreadDumper* threadDumper, 
    const String& suffix, 
    PhysicalResourceArray<Core>& coreList 
){
    ThreadDumper newDumper = 
    {
        new TraceDumper(),
        new VisualizationDumper(),
        new CountDumper()
    };
    *threadDumper = newDumper;

    threadDumper->traceDumper->Initialize( suffix );
    threadDumper->visDumper->Initialize( suffix, coreList );
    threadDumper->countDumper->Initialize( suffix );

    m_dumpEnabled =
        threadDumper->traceDumper->Enabled() || 
        threadDumper->visDumper->IsEnabled()   ||
        threadDumper->countDumper->Enabled() || 
        m_dumpEnabled;
    m_dumperList.push_back( *threadDumper );
}


void Dumper::Initialize(
    PhysicalResourceArray<Core>& coreList,
    PhysicalResourceArray<Thread>& threadList 
){
    LoadParam();

    m_dumpEnabled = false;

    if( m_dumpEachThread ){
        for( int i = 0; i < threadList.GetSize(); i++ ){
            Thread* thread = threadList[i];
            ThreadDumper dumper;
            String suffix = "t" + boost::lexical_cast<String>(i);
            CreateThreadDumper( &dumper, suffix, coreList );
            m_dumperMap[thread] = dumper;
        }
    }
    else if( m_dumpEachCore ){
        for( int i = 0; i < coreList.GetSize(); i++ ){
            Core* core = coreList[i];

            ThreadDumper dumper;
            String suffix = "c" + boost::lexical_cast<String>(i);
            CreateThreadDumper( &dumper, suffix, coreList );

            for( int j = 0; j < core->GetThreadCount(); j++ ){
                Thread* thread = core->GetThread(j);
                m_dumperMap[thread] = dumper;
            }
        }
    }
    else{
        ThreadDumper dumper;
        CreateThreadDumper( &dumper, "", coreList );
        for( int i = 0; i < threadList.GetSize(); i++ ){
            Thread* thread = threadList[i];
            m_dumperMap[thread] = dumper;
        }
    }
}

template <class T> static void DeleteDumper(T*& ptr)
{
    if(ptr){
        delete ptr;
        ptr = NULL;
    }
}

void Dumper::Finalize()
{
    for( DumperList::iterator i = m_dumperList.begin();
        i != m_dumperList.end();
        ++i
    ){
        i->traceDumper->Finalize();
        i->visDumper->Finalize();
        i->countDumper->Finalize();
    }
    ReleaseDumper();
    ReleaseParam();
}

void Dumper::ReleaseDumper()
{
    for( DumperList::iterator i = m_dumperList.begin();
        i != m_dumperList.end();
        ++i
    ){
        DeleteDumper( i->traceDumper );
        DeleteDumper( i->visDumper );
        DeleteDumper( i->countDumper );
    }

    m_dumperList.clear();
    m_dumperMap.clear();
}


void Dumper::DumpStallBeginImpl(OpIterator op)
{
    Thread* thread = op->GetThread();
    ThreadDumper& dumper = m_dumperMap[thread];

    dumper.traceDumper->DumpStallBegin(&*op);
    dumper.visDumper->PrintStallBegin(op);
}

void Dumper::DumpStallEndImpl(OpIterator op)
{
    Thread* thread = op->GetThread();
    ThreadDumper& dumper = m_dumperMap[thread];

    dumper.traceDumper->DumpStallEnd(&*op);
    dumper.visDumper->PrintStallEnd(op);
}

void Dumper::DumpImpl(DUMP_STATE state, OpIterator op, int detail)
{
    if(!m_dumpEnabled)
        return;

    Thread* thread = op->GetThread();
    ThreadDumper& dumper = m_dumperMap[thread];

    dumper.traceDumper->Dump(state, &*op, detail);
    dumper.visDumper->PrintOpState(op, state);
}

void Dumper::SetCurrentCycleImpl( Thread* thread, s64 cycle )
{
    if(!m_dumpEnabled)
        return;

    ThreadDumper& dumper = m_dumperMap[thread];
    dumper.traceDumper->SetCurrentCycle(cycle);
    dumper.visDumper->SetCurrentCycle(cycle);
    dumper.countDumper->SetCurrentCycle(cycle);
}

void Dumper::SetCurrentInsnCountImpl( Thread* thread, s64 count )
{
    if(!m_dumpEnabled)
        return;

    ThreadDumper& dumper = m_dumperMap[thread];
    dumper.countDumper->SetCurrentInsnCount(count);
    dumper.visDumper->SetCurrentInsnCount(thread,count);
}

void Dumper::DumpOpDependencyImpl(
    const OpIterator producerOp, const OpIterator consumerOp, DumpDependency type )
{
    if(!m_dumpEnabled)
        return;

    Thread* thread = producerOp->GetThread();
    ThreadDumper& dumper = m_dumperMap[thread];
    dumper.visDumper->PrintOpDependency( producerOp, consumerOp, type );
}

void Dumper::DumpRawStageImpl( OpIterator op, bool begin, const char*stage, DumpLane lane )
{
    if(!m_dumpEnabled)
        return;

    ASSERT( !op.IsNull(), "A null op is passed." );
    Thread* thread = op->GetThread();
    ThreadDumper& dumper = m_dumperMap[thread];
    dumper.visDumper->PrintRawOutput( op, begin, stage, lane );
}   
void Dumper::SetEnabledImpl( bool enabled )
{
    m_dumpEnabled = enabled;
}


