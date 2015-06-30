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


#ifndef __REGISTERFILE_H__
#define __REGISTERFILE_H__

#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Env/Param/ParamExchange.h"

namespace Onikiri 
{
    class Core;
    class PhyReg;

    class RegisterFile 
        : public PhysicalResourceNode
    {
    private:
        std::vector<PhyReg*> m_register;
        std::vector<int>     m_capacity;
        int m_totalCapacity;
        Core* m_core;
        EmulatorIF* m_emulator;

    public:
        BEGIN_PARAM_MAP( GetParamPath() )
            PARAM_ENTRY("@Capacity",    m_capacity)
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( Core, "core", m_core )
            RESOURCE_ENTRY( EmulatorIF, "emulator", m_emulator )
        END_RESOURCE_MAP()

        RegisterFile();
        virtual ~RegisterFile();

        void Initialize(InitPhase phase);

        PhyReg* GetPhyReg(int phyRegNo) const;
        PhyReg* operator[](int phyRegNo) const;

        void SetPhyReg(int phyRegNo, PhyReg* phyReg);

        int GetCapacity(int segment) const;
        size_t GetSegmentCount() const;
        int GetTotalCapacity() const;
    };

}; // namespace Onikiri

#endif // __REGISTERFILE_H__

