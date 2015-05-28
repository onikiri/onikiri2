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

#include "Sim/Pipeline/Fetcher/Steerer/IcountFetchThreadSteerer.h"

#include "Sim/Thread/Thread.h"
#include "Sim/Op/Op.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/InorderList/InorderList.h"

using namespace Onikiri;

IcountFetchThreadSteerer::IcountFetchThreadSteerer() :
    m_nextThread(0),
    m_evaluatedThread(0)
{
}

IcountFetchThreadSteerer::~IcountFetchThreadSteerer()
{
}

void IcountFetchThreadSteerer::Initialize(InitPhase phase)
{
    if (phase == INIT_PRE_CONNECTION){
        LoadParam();
        return;
    }
    if (phase == INIT_POST_CONNECTION){

        CheckNodeInitialized( "thread", m_thread );

    }
}

void IcountFetchThreadSteerer::Finalize()
{
    ReleaseParam();
}

Thread* IcountFetchThreadSteerer::SteerThread(bool update)
{
    int threadCount = m_thread.GetSize();
    std::vector<int> frontendInsnNum(threadCount,0);

    // Count the number of instructions in front-end for each thread.
    for (int tid = 0; tid < threadCount; tid++)
    {
        InorderList* inorderList = m_thread[tid]->GetInorderList();
        for (OpIterator op = inorderList->GetFrontOp(); op != OpIterator(0); op = inorderList->GetNextIndexOp(op))
        {
            if ( !op->IsDispatched() ||
                (!op->GetOpClass().IsNop() && op->GetScheduler()->IsInScheduler(op)) )
            {
                frontendInsnNum[tid]++;
            }
        }
    }

    // Find active thread
    int currentFetchThread = m_nextThread;
    int count = 0;
    bool found = true;
    while ( !m_thread[currentFetchThread]->IsActive() )
    {
        currentFetchThread = (currentFetchThread + 1) % threadCount;
        count++;
        if( count > threadCount ){
            found = false;
            break;
        }
    }

    if( !found ){
        if( update )
            m_nextThread = currentFetchThread;
        return NULL;
    }

    // Find a thread that has the smallest number of instructions in front-end.
    int targetThread = currentFetchThread;
    count = 0;
    do 
    {
        currentFetchThread = (currentFetchThread + 1) % threadCount;
        if (m_thread[currentFetchThread]->IsActive() && frontendInsnNum[targetThread] > frontendInsnNum[currentFetchThread])
        {
            targetThread = currentFetchThread;
        }
        count++;
    } while ( count <= threadCount );

    if( update ){
        if ( m_evaluatedThread != targetThread )
        {
            targetThread = m_evaluatedThread;
            m_nextThread = (targetThread + 1) % threadCount;
        }
        else
        {
            m_nextThread = (targetThread + 1) % threadCount;
        }
    }
    else
    {
        m_evaluatedThread = targetThread;
    }

    return m_thread[targetThread];
}
