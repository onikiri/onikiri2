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
// Counter based hit/miss predictor 
//

#ifndef SIM_PREDICTOR_HIT_MISS_PRED_COUNTER_BASED_HIT_MISS_PRED_H
#define SIM_PREDICTOR_HIT_MISS_PRED_COUNTER_BASED_HIT_MISS_PRED_H

#include "Types.h"
#include "Lib/shttl/counter_array.h"
#include "Env/Param/ParamExchange.h"

#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/Predictor/HitMissPred/HitMissPredIF.h"

namespace Onikiri
{
    class CounterBasedHitMissPred : 
        public HitMissPredIF,
        public PhysicalResourceNode
    {
    public:
        BEGIN_PARAM_MAP( GetParamPath() )
            PARAM_ENTRY("@CounterBits", m_counterBits)
            PARAM_ENTRY("@EntryBits",   m_entryBits)
            PARAM_ENTRY("@AddressXORConvolute", m_addrXORConvolute)
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
        END_RESOURCE_MAP()

        CounterBasedHitMissPred();
        ~CounterBasedHitMissPred();

        void Initialize(InitPhase phase);

        virtual bool Predict( OpIterator op );
        virtual void Finished( OpIterator op, bool hit ){};
        virtual void Commit( OpIterator op, bool hit );

    private:
        int m_counterBits;
        int m_entryBits;
        bool m_addrXORConvolute;
        shttl::counter_array<u8> m_table;

        u64 ConvolutePCToArrayIndex(PC pc);
        u64 MaskPCToArrayIndex(PC pc);
        u64 GetArrayIndex(PC pc);
    };

} // namespace Onikiri

#endif  // SIM_PREDICTOR_HIT_MISS_PRED_COUNTER_BASED_HIT_MISS_PRED_H

