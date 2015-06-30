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


#ifndef SIM_PREDICTOR_LAT_PRED_LAT_PRED_RESULT_H
#define SIM_PREDICTOR_LAT_PRED_LAT_PRED_RESULT_H

#include "Sim/Op/OpArray/OpArray.h"

namespace Onikiri 
{
    class HitMissPredIF;

    // Result of latency prediction
    class LatPredResult
    {
    public:
        
        static const int MAX_COUNT = 6;

        // A result of latency prediction is returned as a list of 'Scheduling' objects.
        // The following examples are the case of a 3-level memory hierarchy,
        // L1(3cycles) / L2(5cycles) / Mem(8cycles).
        //
        // ( [latency, wakeup] )
        // L1 hit/L2 hit, predicted as
        //   0: [3,    true ]
        //   1: [3+5,  true ]
        //   2: [3+5+8,true ]
        //
        // L1 miss/L2 hit, predicted as
        //   0: [3,    false]
        //   1: [3+5,  true ]
        //   2: [3+5+8,true ]
        //
        // L1 miss/L2 miss, predicted as
        //   0: [3,    false]
        //   1: [3+5,  false]
        //   2: [3+5+8,false]

        struct Scheduling
        {
            int  latency;   // A total latency of this level.
            bool wakeup;    // Consumers are woke up or not at this latency
            
            Scheduling( int l = -1, bool w = false ) :
                latency( l ),
                wakeup ( w )
            {
            }
        };

        LatPredResult() : 
            m_scheduling(),
            m_schedulingCount(-1)
        {
        }

        const Scheduling& Get( int index = 0 ) const
        {
            ASSERT( index < m_schedulingCount );
            return m_scheduling[ index ];
        }

        void Set( int index, const Scheduling& scheduling )
        {
            ASSERT( index < MAX_COUNT );
            m_scheduling[ index ] = scheduling;
        }

        void Set( int index, int latency, bool wakeup )
        {
            Set( index, Scheduling( latency, wakeup ) );
        }

        void SetCount( int count )
        {
            ASSERT( count < MAX_COUNT );
            m_schedulingCount = count;
        }

        int GetCount() const
        {
            return m_schedulingCount;
        }

    protected:
        boost::array< Scheduling, MAX_COUNT > m_scheduling;
        int m_schedulingCount;
    };


}; // namespace Onikiri

#endif // SIM_PREDICTOR_LAT_PRED_LAT_PRED_RESULT_H

