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

#include "Sim/Thread/Thread.h"

#include "Utility/RuntimeError.h"
#include "Sim/Op/Op.h"


using namespace std;
using namespace Onikiri;

Thread::Thread()
{
    m_emulator = 0;
    m_core = 0;
    m_inorderList = 0;
    m_checkpointMaster = 0;
    m_memOrderManager = 0;
    m_regDepPred = 0;
    m_memDepPred = 0;
    
    m_localTID = TID_INVALID;
    m_active = false;
    m_serialOpID = 0;
}

Thread::~Thread()
{
    ReleaseParam();
}

void Thread::Initialize(InitPhase phase)
{
    if(phase == INIT_PRE_CONNECTION){
        LoadParam();
    }
    else if(phase == INIT_POST_CONNECTION){

        CheckNodeInitialized( "checkpointMaster",   m_checkpointMaster );
        CheckNodeInitialized( "emulator",           m_emulator );
        CheckNodeInitialized( "inorderList",        m_inorderList );
        CheckNodeInitialized( "memOrderManager",    m_memOrderManager );
        CheckNodeInitialized( "checkpointMaster",   m_checkpointMaster );
        CheckNodeInitialized( "regDepPred",         m_regDepPred );
        CheckNodeInitialized( "memDepPred",         m_memDepPred );
        CheckNodeInitialized( "core",               m_core );
        CheckNodeInitialized( "recoverer",          m_recoverer );

        // CheckpointedData ÇÃèâä˙âª
        m_fetchPC.Initialize(
            m_checkpointMaster, 
            CheckpointMaster::SLOT_FETCH
        );

        m_retiredOpID.Initialize(
            m_checkpointMaster, 
            CheckpointMaster::SLOT_FETCH
        );
        m_retiredOpID.GetCurrent() = 0;
    }
}

bool Thread::IsActive()
{
    return m_active;
}

void Thread::InitializeContext(PC pc)
{
    int pcTID = pc.tid;
    if( pcTID != GetTID(0) ){
        THROW_RUNTIME_ERROR( 
            "The TID of 'pc' (%d) and the TID of this 'Thread' (%d) are different", 
            pcTID,
            GetTID(0)
        );
    }

    SetFetchPC( pc );
}

int Thread::GetTID()
{
    return GetTID(0);
}

int Thread::GetTID( const int index )
{
    return PhysicalResourceNode::GetTID( index );
}

void Thread::SetLocalThreadID(int localTID)
{
    m_localTID = localTID;
}

int  Thread::GetLocalThreadID() const
{
    return m_localTID;
}

void Thread::Activate( bool active )
{
    m_active = active;
}

void Thread::SetFetchPC(const PC& pc)
{
    ASSERT(
        pc.tid == GetTID(0),
        "The passed pc has invalid tid."
    );
    *m_fetchPC = pc;
}

PC   Thread::GetFetchPC() const
{
    return *m_fetchPC;
}

u64  Thread::GetOpRetiredID()
{
    return *m_retiredOpID;
}

u64  Thread::GetOpSerialID()
{
    return m_serialOpID;
}

void Thread::AddOpRetiredID(u64 num)
{
    *m_retiredOpID = *m_retiredOpID + num;
}

void Thread::AddOpSerialID(u64 num)
{
    m_serialOpID += num;
}

void Thread::SetThreadCount(const int count)
{
    if( count != 1 ){
        THROW_RUNTIME_ERROR( "The tid count of the 'Thread' class must be 1" );
    }
    PhysicalResourceNode::SetThreadCount( count );
}

