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

#include "Sim/Register/RegisterFile.h"
#include "Utility/RuntimeError.h"
#include "Sim/Core/Core.h"

// <FIXME> PhyReg を移動
#include "Sim/Dependency/PhyReg/PhyReg.h"

using namespace Onikiri;

RegisterFile::RegisterFile() :
    m_totalCapacity(0),
    m_core(0),
    m_emulator(0)
{
}

RegisterFile::~RegisterFile()
{
    for (size_t i = 0; i < m_register.size(); i++) {
        if (m_register[i]) {
            delete m_register[i];
            m_register[i] = NULL;
        }
    }
    ReleaseParam();
}

void RegisterFile::Initialize(InitPhase phase)
{
    if(phase == INIT_PRE_CONNECTION){
        LoadParam();
    }
    else if(phase == INIT_POST_CONNECTION){
        // member 変数のチェック
        CheckNodeInitialized( "core", m_core );
        CheckNodeInitialized( "emulator", m_emulator );

        // Check the physical register configuration.
        ISAInfoIF* isaInfo = m_emulator->GetISAInfo();
        if( (size_t)isaInfo->GetRegisterSegmentCount() != m_capacity.size() ){
            THROW_RUNTIME_ERROR(
                "The specified number of the physical register segments (%d) "
                "and that of the logical register segments defined by the ISA (%d) "
                "do not match in the configuration XML. Check the configuration "
                "of the physical registers and the ISA.",
                m_capacity.size(),
                isaInfo->GetRegisterSegmentCount()
            );
        }

        // 全部でいくつ物理レジスタが必要か計算
        m_totalCapacity = 0;
        for(int i = 0; i < static_cast<int>(m_capacity.size()); ++i) {
            m_totalCapacity += m_capacity[i];
        }
        
        // 物理レジスタの確保
        int schedulerCount = m_core->GetNumScheduler();
        m_register.resize(m_totalCapacity, 0);
        for (int i = 0; i < m_totalCapacity; i++) {
            PhyReg* reg = new PhyReg(schedulerCount, i);
            reg->Clear();
            m_register[i] = reg;
        }
    }
}

PhyReg* RegisterFile::GetPhyReg(int phyRegNo) const
{
    ASSERT(phyRegNo >= 0 && phyRegNo < m_totalCapacity,
        "illegal phyRegNo %d.", phyRegNo);
    return m_register[phyRegNo];
}

PhyReg* RegisterFile::operator[](int phyRegNo) const
{
    return GetPhyReg(phyRegNo);
}

void RegisterFile::SetPhyReg(int phyRegNo, PhyReg* phyReg)
{
    ASSERT(phyRegNo >= 0 && phyRegNo < m_totalCapacity,
        "illegal phyRegNo %d.", phyRegNo);
    m_register[phyRegNo] = phyReg;
}

size_t RegisterFile::GetSegmentCount() const
{
    return m_capacity.size();
}

int RegisterFile::GetCapacity(int segment) const
{
    ASSERT(segment >= 0 && segment < static_cast<int>(m_capacity.size()),
        "unknown segment %d", segment);     
    return m_capacity[segment];
}

int RegisterFile::GetTotalCapacity() const
{
    return m_totalCapacity; 
}
