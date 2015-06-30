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


#ifndef SIM_FOUNDATION_CHECK_POINT_CHECKPOINT_H
#define SIM_FOUNDATION_CHECK_POINT_CHECKPOINT_H

#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/Foundation/Checkpoint/CheckpointedDataBase.h"

namespace Onikiri
{

    // Check pointing mechanism.
    // 'Checkpoint' identifies the generation of check pointed data.
    class Checkpoint
    {
    public:

        typedef CheckpointedDataBase::BackupIterator BackupIterator;

        Checkpoint( size_t dataRefSize ) : 
            m_committed( false ),
            m_backedUpEntries( dataRefSize )
        {
        };
        
        ~Checkpoint()
        {
        };

        void Commit()
        {
            m_committed = true;
        }

        bool IsCommitted() const
        {
            return m_committed;
        }

        // Set an iterator that refers actual check pointed data.
        // 'handle' identifies each data.
        // Ex. handle(0):PC, handle(1):RMT ... 
        void SetIterator( CheckpointedDataHandle handle, BackupIterator i )
        {
            ASSERT( handle < m_backedUpEntries.size() );
            Entry* entry = &m_backedUpEntries[ handle ];
            entry->valid = true;
            entry->iterator = i;
        }

        bool GetIterator( const CheckpointedDataHandle handle, BackupIterator* iterator ) const
        {
            ASSERT( m_backedUpEntries.size() > handle, "Invalid handle." );
            const Entry* entry = &m_backedUpEntries[ handle ];
            if( !entry->valid ){
                return false;
            }
            else{
                *iterator = entry->iterator;
                return true;
            }
        }

    protected:
        bool m_committed;
        struct Entry
        {
            BackupIterator iterator;
            bool valid;
            Entry() : 
                valid(false)
            {
            }
        };
        pool_vector< Entry > m_backedUpEntries;
    };


}; // namespace Onikiri

#endif // SIM_FOUNDATION_CHECK_POINT_CHECKPOINT_H

