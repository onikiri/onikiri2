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


#ifndef SIM_FOUNDATION_CHECK_POINT_CHECK_POINT_DATA_H
#define SIM_FOUNDATION_CHECK_POINT_CHECK_POINT_DATA_H

#include "Utility/RuntimeError.h"
#include "Sim/Foundation/Checkpoint/CheckpointMaster.h"
#include "Sim/Foundation/Checkpoint/CheckpointedDataBase.h"
#include "Sim/Foundation/Checkpoint/Checkpoint.h"

namespace Onikiri 
{
    // Implementation of check pointing 'DataType'.
    template <typename DataType>
    class CheckpointedData : public CheckpointedDataBase
    {

    public:
        CheckpointedData() : m_master(0)
        {
        }

        virtual ~CheckpointedData()
        {
            for( BackupIterator i = m_list.begin(); i != m_list.end(); ++i ){
                DataType* ptr = GetData(i);
                Destroy( ptr );
            }
            m_list.clear();
        }

        // This data is registered to 'master' and it is check pointed on 'slot' timing.
        virtual void Initialize( CheckpointMaster* master, CheckpointMaster::Slot slot )
        {
            m_master = master;
            CheckpointedDataHandle handle = master->Register( this, slot );
            SetHandle( handle );
        }

        // Allocate an entry for copying and set the entry to 'checkpoint'.
        virtual void Allocate( Checkpoint* checkpoint )
        {
            // Assign memory for copying.
            BackupEntry entry;
            entry.data = Construct();
            entry.valid = false;

            // Add an allocated entry to the list.
            m_list.push_back( entry );
            BackupIterator i = m_list.end();
            --i;
            
            // Register the iterator of the entry to a checkpoint.
            checkpoint->SetIterator( GetHandle(), i );
        }

        // Current data is backed up to a checkpoint.
        virtual void Backup( Checkpoint* checkpoint )
        {
            BackupIterator entry = GetIterator( checkpoint );
            entry->valid = true;
            *GetData( entry ) = GetCurrent();
        }

        // Current data is recovered from check pointed data.
        virtual void Recover( Checkpoint* checkpoint )
        {
            BackupIterator entry = GetIterator( checkpoint );
            if( entry->valid ){
                // When an entry is invalid, a check point is already allocated but copying 
                // is not done yet. This occurs when an op between fetch and renaming stages
                // is recovered. The op in the stages 'Backup's resources in fetch stages (ex. PC),
                // but resources in renaming stages (ex. RMT) are not backed up yet.
                GetCurrent() = *GetData( entry );
            }
        }

        // Erase data assigned for 'checkpoint'.
        virtual void Erase( Checkpoint* checkpoint )
        {
            BackupIterator i;
            if( GetIterator( checkpoint, &i ) ){
                DataType* ptr = GetData(i);
                if( ptr ){
                    Destroy( ptr );
                }
                m_list.erase( i );
            }
        }
        
        // Accessors returns actual current data.
        DataType&       GetCurrent()        { return m_current;     }
        const DataType& GetCurrent() const  { return m_current;     }
        DataType&       operator*()         { return GetCurrent();  }
        const DataType& operator*() const   { return GetCurrent();  }
        DataType*       operator->()        { return &GetCurrent(); }
        const DataType* operator->() const  { return &GetCurrent(); }

    private:

        DataType   m_current;   // Current data.
        BackupList m_list;      // Check pointed data.

        CheckpointMaster* m_master;

        // An object pool for Check pointed data.
        boost::object_pool<DataType> m_pool;
        void Destroy( DataType* data )
        {
            m_pool.destroy( data );
        }
        DataType* Construct()
        {
            DataType* ptr = m_pool.construct();
            ASSERT( ptr != NULL, "Memory allocation failed." );
            return ptr;
        }

        // Access methods
        bool GetIterator( const Checkpoint* checkpoint, BackupIterator* iterator ) const
        {
            return checkpoint->GetIterator( GetHandle(), iterator );
        }

        BackupIterator GetIterator( const Checkpoint* checkpoint ) const
        {
            BackupIterator iterator;
#if ONIKIRI_DEBUG
            bool success = checkpoint->GetIterator( GetHandle(), &iterator );
            ASSERT( success, "GetIterator() failed." );
#else
            checkpoint->GetIterator( GetHandle(), &iterator );
#endif
            return iterator;
        }

        DataType* GetData( BackupIterator i ) const
        {
            BackupEntry backup = *i;
            return (DataType*)(backup.data);
        }

        DataType* GetData( const Checkpoint* checkpoint ) const
        {
            const BackupIterator& i = GetIterator( checkpoint );
            return GetData(i);
        }
    };

}; // namespace Onikiri

#endif // SIM_FOUNDATION_CHECK_POINT_CHECK_POINT_DATA_H

