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


// Op class statistics.
// This class is a utility class that maintains statistics such as 
// the number of each op class and that of source operands.

#ifndef SIM_OP_OP_CLASS_STATISTICS_H
#define SIM_OP_OP_CLASS_STATISTICS_H

#include "Env/Param/ParamExchange.h"
#include "Sim/Op/OpArray/OpArray.h"

namespace Onikiri
{
    class OpClassStatistics : public ParamExchangeChild
    {
    public:
        BEGIN_PARAM_MAP("")
            RESULT_ENTRY("@NumOpCode", m_numOps)
            RESULT_ENTRY("@NameOpCode", m_names)
            RESULT_ENTRY("@NumIntSrcOperands", m_numIntSrcOperands)
            RESULT_ENTRY("@NumIntDstOperands", m_numIntDstOperands)
            RESULT_ENTRY("@NumFpSrcOperands", m_numFpSrcOperands)
            RESULT_ENTRY("@NumFpDstOperands", m_numFpDstOperands)
        END_PARAM_MAP()

        // Constructor.
        OpClassStatistics();

        // Increment a counter corresponding to 'op'.
        void Increment(OpIterator op);

        // Reset statistics counters.
        void Reset();

    private:
        std::vector<s64> m_numOps;
        std::vector<const char*> m_names;

        s64 m_numIntSrcOperands;
        s64 m_numIntDstOperands;
        s64 m_numFpSrcOperands;
        s64 m_numFpDstOperands;
    };

    

}; // namespace Onikiri

#endif // SIM_OP_OP_CLASS_STATISTICS_H

