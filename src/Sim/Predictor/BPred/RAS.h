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


#ifndef __RAS_H__
#define __RAS_H__

#include "Interface/Addr.h"
#include "Interface/OpStateIF.h"
#include "Sim/Foundation/Checkpoint/CheckpointedData.h"

#include "Env/Param/ParamExchange.h"
#include "Sim/Foundation/Resource/ResourceNode.h"

namespace Onikiri 
{
    class SimPC;

    // return address stack
    class RAS : public PhysicalResourceNode
    {
    private:
        CheckpointMaster*     m_checkpointMaster;
        CheckpointedData<int> m_stackTop; // StackÇÃÉgÉbÉvÇÃèÍèäÇé¶Ç∑
        std::vector<PC>       m_stack;

        CheckpointedData<int> m_backupStackTop;
        static const int MAX_BACKUP_SIZE = 1;
        typedef boost::array< PC, MAX_BACKUP_SIZE > BackupStack;
        CheckpointedData< BackupStack > m_backupStack;
        bool m_enableBackup;

        int m_stackSize;
    public:
        // parameter mapping
        BEGIN_PARAM_MAP( GetParamPath() )
            PARAM_ENTRY( "@StackSize", m_stackSize )
            PARAM_ENTRY( "@EnableBackup", m_enableBackup )
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( CheckpointMaster, "checkpointMaster", m_checkpointMaster )
        END_RESOURCE_MAP()

        RAS();
        virtual ~RAS();

        void Initialize(InitPhase phase);

        // call ñΩóﬂÇÃPCÇpush
        void Push(const SimPC& pc);
        
        // return ñΩóﬂÇ»ÇÃÇ≈PCÇPop
        SimPC Pop();

        // accessors
        int GetStackSize() { return m_stackSize; }

        int PointStackPos(int basePos, int stackSize, int offset) 
        { return (basePos + stackSize + offset) % stackSize; }
    };

}; // namespace Onikiri

#endif // __RAS_H__

