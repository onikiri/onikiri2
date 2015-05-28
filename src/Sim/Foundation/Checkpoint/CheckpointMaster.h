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


#ifndef SIM_FOUNDATION_CHECK_POINT_CHECK_POINT_MASTER_H
#define SIM_FOUNDATION_CHECK_POINT_CHECK_POINT_MASTER_H

#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/Foundation/Checkpoint/CheckpointedDataBase.h"

namespace Onikiri
{
    class Checkpoint;

    // A class managing check-pointed data.
    // 'Checkpoint' is a mechanism that backs up states and
    // recovers on mis-prediction.
    class CheckpointMaster : public PhysicalResourceNode
    {
    public:
        typedef pool_list< Checkpoint* > CheckpointListType;
        typedef CheckpointListType::iterator CheckpointListIterator;

        typedef pool_vector< CheckpointedDataBase* > CheckpointedDataListType;
        typedef CheckpointedDataListType::iterator CheckpoinedtDataListIterator;

        enum Slot 
        {
            SLOT_FETCH = 0,
            SLOT_RENAME,
            SLOT_MAX
        };

        // parameter mapping
        BEGIN_PARAM_MAP( GetParamPath() )
            PARAM_ENTRY("@Capacity", m_capacity)
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
        END_RESOURCE_MAP()

        CheckpointMaster();
        virtual ~CheckpointMaster();

        void Initialize( InitPhase phase );

        // Register check pointed data.
        // This method must be called from CheckpointedData::Initialize().
        CheckpointedDataHandle Register( CheckpointedDataBase* data, Slot slot );

        // Create a new checkpoint.
        // Returns the pointer of a checkpoint that identifies a generation of check-pointed data.
        Checkpoint* CreateCheckpoint();

        // Current data is becked up to 'checkpoint'.
        void Backup( Checkpoint* checkpoint, Slot slot );

        // Commits 'checkpoint'.
        void Commit( Checkpoint* checkpoint );

        // Flushes 'checkpoint'.
        void Flush( Checkpoint* checkpoint );

        // Recover current data to check-pointed data of 'checkpoint'.
        void Recover( Checkpoint* checkpoint );

        // accessors
        bool CanCreate( int num ) const
        {
            return m_capacity >= m_checkpoint.size() + num; 
        }

    protected:
        size_t m_capacity;  // The maximum number of checkpoints.

        CheckpointedDataListType m_data;
        std::vector<CheckpointedDataListType> m_dataTable;  // Indexed by Slot

        CheckpointListType m_checkpoint;
        
        // An object pool for checkpoints.
        boost::object_pool<Checkpoint> m_checkpointPool;
        Checkpoint* ConstructCheckpoint( size_t refSize );
        void DestroyCheckpoint( Checkpoint* cp );

    };

}; // namespace Onikiri

#endif // SIM_FOUNDATION_CHECK_POINT_CHECK_POINT_MASTER_H

