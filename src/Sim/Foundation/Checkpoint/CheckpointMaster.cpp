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

#include "Sim/Foundation/Checkpoint/CheckpointMaster.h"

#include "Sim/Foundation/Checkpoint/CheckpointedData.h"
#include "Utility/RuntimeError.h"
#include "Sim/Core/Core.h"
#include "Sim/Core/DataPredTypes.h"


using namespace std;
using namespace Onikiri;

CheckpointMaster::CheckpointMaster() : 
    m_capacity(0)
{
}

CheckpointMaster::~CheckpointMaster()
{
    // Release all checkpoints and related data.
    for( CheckpointListIterator i = m_checkpoint.begin(); i != m_checkpoint.end(); ++i ){
        DestroyCheckpoint( *i );
    }
    m_checkpoint.clear();

    ReleaseParam();
}

void CheckpointMaster::Initialize( InitPhase phase )
{
    if( phase == INIT_PRE_CONNECTION ){
        LoadParam();
        m_dataTable.resize( SLOT_MAX );
    }
    
}

// Register check pointed data.
// This method must be called from CheckpointedData::Initialize().
CheckpointedDataHandle CheckpointMaster::Register(
    CheckpointedDataBase* data,
    Slot slot
){
    m_data.push_back( data );
    m_dataTable[ slot ].push_back( data );
    return m_data.size() - 1;
}

// Create a new checkpoint.
// Returns the pointer of a checkpoint that identifies a generation of check-pointed data.
Checkpoint* CheckpointMaster::CreateCheckpoint()
{
    ASSERT( m_capacity > m_checkpoint.size(), "cannot create checkpoint." );

    Checkpoint* cp = ConstructCheckpoint( m_data.size() );
    m_checkpoint.push_back( cp );

    for( CheckpointedDataListType::iterator i = m_data.begin(); i != m_data.end(); ++i ){
        (*i)->Allocate( cp );
    }
    return cp;
}

// Current data is becked up to 'checkpoint'.
void CheckpointMaster::Backup( Checkpoint* checkpoint, Slot slot )
{
    ASSERT( checkpoint != NULL );
    CheckpointedDataListType* copyTo = &m_dataTable[ slot ];
    for( CheckpointedDataListType::iterator i = copyTo->begin(); i != copyTo->end(); ++i ){
        (*i)->Backup( checkpoint ); 
    }
}

// Commits 'checkpoint'.
// Ops backs up states are committed and their checkpoints are released.
void CheckpointMaster::Commit( Checkpoint* checkpoint )
{
    ASSERT( 
        m_checkpoint.size() > 0 && m_checkpoint.front() == checkpoint,
        "A checkpoint that is not at the front is committed. Checkpoints must be flushed in-order."
    );
    DestroyCheckpoint( checkpoint );
    m_checkpoint.pop_front();
}

// Flushes 'checkpoint'.
void CheckpointMaster::Flush( Checkpoint* checkpoint )
{
    ASSERT(
        m_checkpoint.size() > 0 && m_checkpoint.back() == checkpoint,
        "A checkpoint that is not at the back is flushed. Checkpoints must be flushed from the back."
    );
    DestroyCheckpoint( checkpoint );
    m_checkpoint.pop_back();
}

// Recover current data to check-pointed data of 'checkpoint'.
void CheckpointMaster::Recover( Checkpoint* checkpoint )
{
    for( CheckpointedDataListType::iterator i = m_data.begin(); i != m_data.end(); ++i ){
        (*i)->Recover( checkpoint );
    }
}

// Methods for an object pool of checkpoints.
Checkpoint* CheckpointMaster::ConstructCheckpoint( size_t refSize )
{
    Checkpoint* ptr = m_checkpointPool.construct( m_data.size() );
    ASSERT( ptr != NULL, "Memory allocation failed." );
    return ptr;
}

void CheckpointMaster::DestroyCheckpoint( Checkpoint* checkpoint )
{
    // Erase data referred from checkpoint.
    for( CheckpointedDataListType::iterator i = m_data.begin(); i != m_data.end(); ++i ){
        (*i)->Erase( checkpoint ); 
    }

    m_checkpointPool.destroy( checkpoint );
}
