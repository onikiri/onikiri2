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

#include "Sim/Predictor/DepPred/RegDepPred/RegDepPredBase.h"
#include "Sim/Op/Op.h"

using namespace Onikiri;

RegDepPredBase::RegDepPredBase()
{
}

RegDepPredBase::~RegDepPredBase()
{
}


// 依存関係を解決して、opにセット
// どう解決するかはResolveRegの実装
void RegDepPredBase::Resolve(OpIterator op)
{
    OpInfo* opInfo = op->GetOpInfo();
    int srcNum = opInfo->GetSrcNum();
    for(int i = 0; i < srcNum; ++i) {
        int operand = opInfo->GetSrcOperand(i);
        
        if( operand == -1 ) {
            continue;
        }

        int phyRegNo = ResolveReg(operand);
        op->SetSrcReg(i, phyRegNo);
    }
}

// 新しいレジスタのインスタンスを割り当てて、opにセット
// resolve のために割り当てを記憶しておくのは AllocateReg の実装
void RegDepPredBase::Allocate(OpIterator op)
{
    OpInfo* opInfo = op->GetOpInfo();
    int dstNum = opInfo->GetDstNum();
    for(int i = 0; i < dstNum; ++i) {
        int operand = opInfo->GetDstOperand(i);
        
        if( operand == -1 ) {
            continue;
        }

        int phyRegNo = AllocateReg(op,operand);
        op->SetDstReg(i, phyRegNo);
    }
}

// Commit時、同じ論理レジスタ番号に書き込む直前の命令のデスティネーション・レジスタを解放
void RegDepPredBase::Commit(OpIterator op)
{
    OpInfo* opInfo = op->GetOpInfo();
    int dstNum = opInfo->GetDstNum();
    for(int i = 0; i < dstNum; ++i) {
        int operand = opInfo->GetDstOperand(i);
        ReleaseReg(op, operand, op->GetDstReg(i));
    }
}

// OpがフラッシュされたのでOp自身のデスティネーション・レジスタを解放
void RegDepPredBase::Flush(OpIterator op)
{
    if( op->GetStatus() == OpStatus::OS_FETCH ){
        return;
    }
    OpInfo* opInfo = op->GetOpInfo();
    int dstNum = opInfo->GetDstNum();
    for(int i = 0; i < dstNum; ++i) {
        int operand = opInfo->GetDstOperand(i);
        DeallocateReg(op, operand, op->GetDstReg(i));
    }
}
