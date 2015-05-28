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


#ifndef SIM_FOUNDATION_CHECK_POINT_CHECKPOINTED_DATA_BASE_H
#define SIM_FOUNDATION_CHECK_POINT_CHECKPOINTED_DATA_BASE_H


namespace Onikiri
{
    // Handles are used for identifying each data.
    // Ex. handle(0):PC, handle(1):RMT ... 
    typedef size_t CheckpointedDataHandle;

    class CheckpointMaster;
    class Checkpoint;

    // A base class of check pointed data.
    // See comments in CheckpointedData.
    class CheckpointedDataBase 
    {
    public:
        struct BackupEntry
        {
            void* data;
            bool  valid;
            BackupEntry() :
                data(NULL),
                valid(false)
            {
            }
        };

        typedef pool_list< BackupEntry > BackupList;
        typedef BackupList::iterator BackupIterator;

        CheckpointedDataBase(){}
        virtual ~CheckpointedDataBase(){}

        virtual void Allocate( Checkpoint* checkpoint ) = 0;
        virtual void Backup( Checkpoint* checkpoint ) = 0;
        virtual void Recover( Checkpoint* checkpoint ) = 0;
        virtual void Erase( Checkpoint* checkpoint ) = 0;

        void SetHandle( CheckpointedDataHandle handle )
        {
            m_handle = handle;
        }

        const CheckpointedDataHandle GetHandle() const
        {
            return m_handle;
        }

    protected:
        CheckpointedDataHandle m_handle;

    };
}; // namespace Onikiri

#endif // SIM_FOUNDATION_CHECK_POINT_CHECKPOINTED_DATA_BASE_H

