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

#include "Sim/System/EmulationDebugSystem/EmulationDebugSystem.h"
#include "Sim/System/EmulationDebugSystem/EmulationDebugOp.h"
#include "Sim/System/EmulationDebugSystem/DebugStub/DebugStub.h"
#include "Interface/EmulatorIF.h"

using namespace std;
using namespace boost;
using namespace Onikiri;


void EmulationDebugSystem::Run( SystemContext* context )
{
    int processCount = context->emulator->GetProcessCount();
    if( processCount != 1 ){
        THROW_RUNTIME_ERROR( 
            "Remote debugging is not supported in the execution of multi processes." 
        );
    }

    // レジスタの初期化
    ArchitectureStateList& archStateList = context->architectureStateList;

    s64 insnCount = 0;
    int curPID = 0;
    int terminateProcesses = 0;
    vector<u64> opID( processCount );

    m_debugStub = new DebugStub(context, curPID);

    while( insnCount < context->executionInsns ){

        // Decide a thread executed in this iteration.
        PC& curThreadPC = archStateList[curPID].pc;

        if( curThreadPC.address == 0 ) {
            terminateProcesses++;
            if(terminateProcesses >= processCount){
                break;
            }
            else{
                continue;
            }
        }

        EmulationDebugOp op( context->emulator->GetMemImage() );

        // このPC
        std::pair<OpInfo**, int> ops = 
            context->emulator->GetOp( curThreadPC );
        OpInfo** opInfoArray = ops.first;
        int opCount          = ops.second;

        // opInfoArray の命令を全て実行
        for(int opIndex = 0; opIndex < opCount; opIndex ++){
            OpInfo* opInfo = opInfoArray[opIndex];
            op.SetPC( curThreadPC );
            op.SetTakenPC( Addr(curThreadPC.pid, curThreadPC.tid, curThreadPC.address+4) );
            op.SetOpInfo(opInfo);
            op.SetTaken(false);

            // ソースオペランドを設定
            int srcCount = opInfo->GetSrcNum();
            for (int i = 0; i < srcCount; i ++) {
                op.SetSrc(i, archStateList[curPID].registerValue[ opInfo->GetSrcOperand(i) ] );
            }

            context->emulator->Execute( &op, opInfo );

            // 命令の結果を取得
            int dstCount = opInfo->GetDstNum();
            for (int i = 0; i < dstCount; i ++) {
                archStateList[curPID].registerValue[ opInfo->GetDstOperand(i) ] = op.GetDst(i);
            }

            ++opID[curPID];
        }

        // 次のPC
        if (op.GetTaken())
            curThreadPC = op.GetTakenPC();
        else
            curThreadPC.address += SimISAInfo::INSTRUCTION_WORD_BYTE_SIZE;

        insnCount ++;
        curPID = (curPID + 1) % processCount;   // Round robin
        m_debugStub->OnExec(&op);

    }
    
    delete m_debugStub;

    context->executedInsns.clear();
    context->executedInsns.push_back( insnCount );
    context->executedCycles = insnCount;
}

