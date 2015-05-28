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

#include "Sim/Register/RegisterFreeList.h"
#include "Sim/Core/Core.h"
#include "Sim/Register/RegisterFile.h"

using namespace std;
using namespace Onikiri;

RegisterFreeList::RegisterFreeList()
{
}

RegisterFreeList::~RegisterFreeList()
{
    ReleaseParam();
}

void RegisterFreeList::Initialize(InitPhase phase)
{
    if(phase == INIT_PRE_CONNECTION){
        LoadParam();
    }
    else if(phase == INIT_POST_CONNECTION){
        // メンバ変数のチェック
        if( m_registerFile.GetSize() == 0 ) {
            THROW_RUNTIME_ERROR("registerFile not set.");
        }
        if( m_emulator.GetSize() == 0 ) {
            THROW_RUNTIME_ERROR("emulator not set.");
        }

        ISAInfoIF* isaInfo = m_emulator[0]->GetISAInfo();

        int numLogicalReg = isaInfo->GetRegisterCount();


        // segment の番号をリスト化するためのset
        set<int> segmentSet;
        for (int i = 0; i < numLogicalReg; ++i) {
            int segment = isaInfo->GetRegisterSegmentID(i);
            segmentSet.insert(segment);
        }

        // freeList の初期化
        m_freeList.resize(segmentSet.size());

        // freeList はsegment ごとに構築
        int phyRegNo = 0;
        for(set<int>::iterator iter = segmentSet.begin();
            iter != segmentSet.end();
            ++iter)
        {
            int segment = *iter;
            int capacity = m_registerFile[0]->GetCapacity(segment);
            // freeList に登録
            for (int i = 0; i < capacity; i++) {
                m_freeList[segment].push_back(phyRegNo);
                ++phyRegNo;
            }
        }

        ASSERT(
            phyRegNo == m_registerFile[0]->GetTotalCapacity(),
            "phyreg count unmatch"
        );


        // フリーリストの空きエントリをチェック
        
        vector<int> logicalRegNumList(segmentSet.size(), 0);
        for (int i = 0; i < numLogicalReg; i++) {
            int segment = isaInfo->GetRegisterSegmentID(i);
            logicalRegNumList[segment]++;
        }

        int threadCount = m_core[0]->GetThreadCount();
        for( size_t i = 0; i < segmentSet.size(); i++ ){
            int requiredRegCount = logicalRegNumList[i] * threadCount + 1;
            if( GetFreeEntryCount((int)i) < requiredRegCount ){
                THROW_RUNTIME_ERROR(
                    "The number of the physical register is too small.\n"
                    "segment: %d\n"
                    "phy reg num: %d\n"
                    "requried num: %d (log reg num:%d x thread num:%d + 1)\n",
                    i,
                    m_registerFile[0]->GetCapacity((int)i),
                    requiredRegCount,
                    logicalRegNumList[i],
                    threadCount
                );
            }
        }
    }
}

void RegisterFreeList::Release(int segment, int phyRegNum)
{
    m_freeList[segment].push_back(phyRegNum);
}

int RegisterFreeList::Allocate(int segment)
{
    ASSERT( m_freeList[segment].size() > 0, "cannot allocate register.");

    int phyRegNo = m_freeList[segment].front();
    m_freeList[segment].pop_front();

    return phyRegNo;
}

int RegisterFreeList::GetSegmentCount()
{
    return (int)m_freeList.size();
}

int RegisterFreeList::GetFreeEntryCount(int segment)
{
    return (int)m_freeList[segment].size();
}
