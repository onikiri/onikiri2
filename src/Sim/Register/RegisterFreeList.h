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


#ifndef __REGISTERFREE_LIST_H__
#define __REGISTERFREE_LIST_H__

#include "Sim/Foundation/Resource/ResourceNode.h"

namespace Onikiri 
{
    class RegisterFile;
    class Core;
    class EmulatorIF;

    class RegisterFreeList 
        : public PhysicalResourceNode
    {
        // フリーリスト
        // Allocationが可能な物理レジスタの番号を管理する
        std::vector< pool_list<int> > m_freeList;

        // emulator
        PhysicalResourceArray<EmulatorIF> m_emulator;

        // レジスタ本体
        PhysicalResourceArray<RegisterFile> m_registerFile;

        // Core
        PhysicalResourceArray<Core> m_core;


    public:

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( Core,           "core",         m_core )
            RESOURCE_ENTRY( EmulatorIF,     "emulator",     m_emulator )
            RESOURCE_ENTRY( RegisterFile,   "registerFile", m_registerFile )
        END_RESOURCE_MAP()

        RegisterFreeList();
        virtual ~RegisterFreeList();

        void Initialize(InitPhase phase);

        void Release(int segment, int phyRegNum);
        int Allocate(int segment);

        int GetSegmentCount();
        int GetFreeEntryCount(int segment);
    };

}; // namespace Onikiri

#endif // __REGISTERFREE_LIST_H__

