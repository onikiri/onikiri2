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


//
// Hit/miss predictor 
//

#include <pch.h>
#include "Sim/Predictor/HitMissPred/CounterBasedHitMissPred.h"
#include "Sim/Core/Core.h"
#include "Sim/Op/Op.h"

using namespace Onikiri;

CounterBasedHitMissPred::CounterBasedHitMissPred() :
    m_counterBits(0), 
    m_entryBits(0)
{
}
    
CounterBasedHitMissPred::~CounterBasedHitMissPred()
{
    ReleaseParam();
}

void CounterBasedHitMissPred::Initialize(InitPhase phase)
{
    if(phase == INIT_PRE_CONNECTION){
        LoadParam();

        u8 max = (1 << m_counterBits) - 1;
        m_table.construct(
            (u64)1 << m_entryBits,   // size
            (max + 1) / 2,      // init
            0,                  // min
            max,                // max
            1,                  // add
            2,                  // sub
            (max + 1) / 2       // threshold
        );
    }
}

u64 CounterBasedHitMissPred::ConvolutePCToArrayIndex(PC pc)
{
    int shift = m_core->GetInstructionWordByteShift();
    u64 p = pc.address >> shift;
    p ^= pc.tid;
    return shttl::xor_convolute(p, m_entryBits);
    //return mask(pc.address, m_entryBits);
}

u64 CounterBasedHitMissPred::MaskPCToArrayIndex(PC pc)
{
    int shift = m_core->GetInstructionWordByteShift();
    u64 p = pc.address >> shift;
    p ^= pc.tid;
    return shttl::mask(0, m_entryBits) & p; 
}

u64 CounterBasedHitMissPred::GetArrayIndex(PC pc)
{
    if( m_addrXORConvolute ) {
        return ConvolutePCToArrayIndex(pc);
    }else {
        return MaskPCToArrayIndex(pc); 
    }
}

void CounterBasedHitMissPred::Commit( OpIterator op, bool hit )
{
    size_t index = (size_t)GetArrayIndex( op->GetPC() );
    if(hit)
        m_table[index].inc();
    else
        m_table[index].dec();
}

bool CounterBasedHitMissPred::Predict( OpIterator op )
{
    size_t index = (size_t)GetArrayIndex( op->GetPC() );
    return m_table[index].above_threshold();
}

