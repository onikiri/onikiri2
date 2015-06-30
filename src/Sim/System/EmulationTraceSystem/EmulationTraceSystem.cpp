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

#include "Sim/System/EmulationTraceSystem/EmulationTraceSystem.h"
#include "Sim/System/EmulationSystem/EmulationOp.h"
#include "Emu/EmulatorFactory.h"

using namespace std;
using namespace boost;
using namespace Onikiri;


void EmulationTraceSystem::Run( SystemContext* context )
{
    int processCount = context->emulator->GetProcessCount();

    vector<ofstream*> ofsList;
    ofsList.resize( processCount );
    for( int pid = 0; pid < processCount; pid++ ){
        String fileName = 
            g_env.GetHostWorkPath() + "./emulator." + lexical_cast<String>(pid) + ".log";
        ofsList[pid] = new ofstream( fileName );
    }

    // レジスタの初期化
    ArchitectureStateList& archStateList = context->architectureStateList;

    s64 totalInsnCount = 0;
    vector<s64> insnCount;
    insnCount.resize( processCount, 0 );

    int curPID = 0;
    int terminateProcesses = 0;
    vector<u64> opID( processCount );

    while( totalInsnCount < context->executionInsns ){

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

        ofstream& ofs = *ofsList[curPID];
        EmulationOp op( context->emulator->GetMemImage() );

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
            context->emulator->Commit( &op, opInfo );

            // 命令の結果を取得
            int dstCount = opInfo->GetDstNum();
            for (int i = 0; i < dstCount; i ++) {
                archStateList[curPID].registerValue[ opInfo->GetDstOperand(i) ] = op.GetDst(i);
            }

            // 出力
            ofs << "ID: " << opID[curPID] << "\tPC: " << curThreadPC.pid << "/" << hex << curThreadPC.address << dec << "[" << opIndex << "]\t";
            for (int i = 0; i < opInfo->GetDstNum(); ++i) {
                ofs << "d" << i << ": " << opInfo->GetDstOperand(i) << "\t";
            }
            for (int i = opInfo->GetDstNum(); i < SimISAInfo::MAX_DST_REG_COUNT; ++i) {
                ofs << "d" << i << ": -1" << "\t";
            }

            for (int i = 0; i < opInfo->GetSrcNum(); ++i) {
                ofs << "s" << i << ": " << opInfo->GetSrcOperand(i) << "\t";
            }
            for (int i = opInfo->GetSrcNum(); i < SimISAInfo::MAX_SRC_REG_COUNT; ++i) {
                ofs << "s" << i << ": -1" << "\t";
            }

            ofs << "TPC: " << op.GetTakenPC().pid << "/" << hex << op.GetTakenPC().address << dec << "(" << ( op.GetTaken() ? "t" : "n" ) << ")\t";


            for (int i = 0; i < opInfo->GetDstNum(); ++i) {
                if( opInfo->GetDstOperand(i) != -1 ) {
                    ofs << "r" << opInfo->GetDstOperand(i) << "= " << hex << op.GetDst(i) << dec << "\t";
                }else {
                    ofs << "r_= 0\t" ; 
                }
            }
            for (int i = opInfo->GetDstNum(); i < SimISAInfo::MAX_DST_REG_COUNT; ++i) {
                ofs << "r_= 0\t" ; 
            }

            for (int i = 0; i < opInfo->GetSrcNum(); ++i) {
                if( opInfo->GetSrcOperand(i) != -1 ) {
                    ofs << "r" << opInfo->GetSrcOperand(i) << "= " << hex << op.GetSrc(i) << dec  << "\t";
                }else {
                    ofs << "r_= 0\t" ; 
                }
            }
            for (int i = opInfo->GetSrcNum(); i < SimISAInfo::MAX_SRC_REG_COUNT; ++i) {
                ofs << "r_= 0\t" ; 
            }

            /*
            ofs << "Mem: " << hex << op.GetMemAccess().address.address << dec << "/"
            << op.GetMemAccess().size << "/"
            << (op.GetMemAccess().sign ? "s" : "u") << "/"
            << op.GetMemAccess().value;
            */
            ofs << endl;
            ++opID[curPID];
        }

        // 次のPC
        if (op.GetTaken())
            curThreadPC = op.GetTakenPC();
        else
            curThreadPC.address += SimISAInfo::INSTRUCTION_WORD_BYTE_SIZE;

        totalInsnCount++;
        insnCount[ curPID ]++;
        curPID = (curPID + 1) % processCount;   // Round robin
    }

    for( int pid = 0; pid < processCount; pid++ ){
        ofsList[pid]->close();
        delete ofsList[pid];
    }

    context->executedInsns  = insnCount;
    context->executedCycles = 0;
}

