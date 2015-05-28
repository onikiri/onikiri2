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

#include "Sim/System/EmulationSystem/EmulationSystem.h"

using namespace std;
using namespace boost;
using namespace Onikiri;

void EmulationSystem::Run( SystemContext* context )
{
    s64 numInsns     = context->executionInsns;
    int processCount = context->emulator->GetProcessCount();

    // 論理レジスタの初期値を emulator に教えてもらってセット
    ArchitectureStateList& archStateList = context->architectureStateList;

    // 実行
    vector<s64> totalInsnCount;
    u64 executeInsns = numInsns / processCount;
    for( int pid = 0; pid < processCount; pid++ ){
        u64 insnCount = 0;
        archStateList[pid].pc = 
            context->emulator->Skip(
                archStateList[pid].pc,
                executeInsns,
                &archStateList[pid].registerValue[0],
                &insnCount, 
                NULL
            );
        totalInsnCount.push_back( insnCount );
    }

    context->executedInsns  = totalInsnCount;
    context->executedCycles = 0;
}
