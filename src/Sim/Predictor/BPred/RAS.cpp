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

#include "Sim/Predictor/BPred/RAS.h"
#include "Sim/ISAInfo.h"
#include "Sim/Foundation/SimPC.h"

using namespace Onikiri;

RAS::RAS()
{
    m_checkpointMaster = 0;
}

RAS::~RAS()
{
    ReleaseParam();
}

void RAS::Initialize(InitPhase phase)
{
    if(phase == INIT_PRE_CONNECTION){
        LoadParam();

        ASSERT(m_stackSize >= MAX_BACKUP_SIZE,
            "The size of backup of RAS must be the same or smaller than that of RAS itself.");

        ASSERT(MAX_BACKUP_SIZE > 0,
            "MAX_BACKUP_SIZE must be larger than 0.");

        // 空のpcStackを生成する
        m_stack.resize( m_stackSize, PC() );
    }
    else if(phase == INIT_POST_CONNECTION){

        // checkpointMaster がセットされているかのチェック
        CheckNodeInitialized( "checkpointMaster", m_checkpointMaster );

        m_stackTop.Initialize(
            m_checkpointMaster,
            CheckpointMaster::SLOT_FETCH
        );
        *m_stackTop = 0;

        m_backupStackTop.Initialize(
            m_checkpointMaster,
            CheckpointMaster::SLOT_FETCH
            );
        *m_backupStackTop = 0;
        m_backupStack.Initialize(
            m_checkpointMaster,
            CheckpointMaster::SLOT_FETCH
            );

    }
    
}

// call命令のPCをPushする
void RAS::Push(const SimPC& pc)
{
    if (m_enableBackup)
    {
        // Set the oldest PC in the checkpointed stack to the actual RAS.
        int setRasStackPos = PointStackPos(*m_stackTop, GetStackSize(), -MAX_BACKUP_SIZE);
        m_stack[setRasStackPos] = (*m_backupStack)[*m_backupStackTop];

        // Push a new PC.
        (*m_backupStack)[*m_backupStackTop] = pc.Next();
        *m_backupStackTop = PointStackPos(*m_backupStackTop, MAX_BACKUP_SIZE, 1);
    }

    // Push a new PC.
    m_stack[*m_stackTop] = pc.Next();
    *m_stackTop = PointStackPos(*m_stackTop, GetStackSize(), 1);
}

// return命令なのでPCをPOPする
SimPC RAS::Pop()
{
    // Pop the latest PC.
    *m_stackTop = PointStackPos(*m_stackTop, GetStackSize(), -1);
    PC popPC = m_stack[*m_stackTop];

    if ( m_enableBackup )
    {
        // Pop the latest PC.
        *m_backupStackTop = PointStackPos(*m_backupStackTop, MAX_BACKUP_SIZE, -1);
        popPC = (*m_backupStack)[*m_backupStackTop];

        // Set the latest PC that the checkpointed does not have.
        int setBackupStackPos = PointStackPos(*m_stackTop, GetStackSize(), -MAX_BACKUP_SIZE);
        (*m_backupStack)[*m_backupStackTop] = m_stack[setBackupStackPos];
    }

    return popPC;
}
