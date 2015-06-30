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


#ifndef __BTB_H__
#define __BTB_H__

#include "Lib/shttl/setassoc_table.h"
#include "Interface/Addr.h"

#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Env/Param/ParamExchange.h"
#include "Sim/ISAInfo.h"

#include "Sim/Predictor/BPred/BranchType.h"

namespace Onikiri 
{
    // Prediction result of BTB
    struct BTBPredict
    {
        PC            predIndexPC;
        PC            target;
        BranchType    type;
        bool          hit;
        bool          dirPredict;
    };

    class BTB : public PhysicalResourceNode
    {
    private:
        static const int WORD_BITS = SimISAInfo::INSTRUCTION_WORD_BYTE_SHIFT;
        typedef shttl::static_off_hasher<u64, WORD_BITS> HasherType;
        typedef shttl::setassoc_table< std::pair<u64, BTBPredict>, HasherType> SetAssocTableType;
        SetAssocTableType* m_table;

        int m_numEntryBits;
        int m_numWays;

        // statistical information
        s64     m_numPred;          // fetch時に予測した回数
        s64     m_numTableHit;      // テーブルにヒットした回数
        s64     m_numTableMiss;     // テーブルにミスした回数
        s64     m_numHit;           // ターゲットがヒットした回数
        s64     m_numMiss;          // ターゲットがミスした回数
        s64     m_numUpdate;        // Updateした命令数
        int     m_numEntries;       // エントリ数

    public:
        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY("@EntryBits",   m_numEntryBits)
                PARAM_ENTRY("@Ways",        m_numWays)
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
                PARAM_ENTRY( "@NumPred",          m_numPred)
                PARAM_ENTRY( "@NumUpdate",    m_numUpdate)
                PARAM_ENTRY( "@NumTableHit",      m_numTableHit)
                PARAM_ENTRY( "@NumTableMiss",  m_numTableMiss)
                PARAM_ENTRY( "@NumHit",        m_numHit)
                PARAM_ENTRY( "@NumMiss",       m_numMiss)
                PARAM_ENTRY( "@NumEntries",   m_numEntries)
                RESULT_RATE_SUM_ENTRY( "@TableHitRate", m_numTableHit, m_numTableHit, m_numTableMiss )
                RESULT_RATE_SUM_ENTRY( "@HitRate", m_numHit, m_numHit, m_numMiss )
            END_PARAM_PATH()
        END_PARAM_MAP()
        
        BEGIN_RESOURCE_MAP()
        END_RESOURCE_MAP()

        using PhysicalResourceNode::LoadParam;
        using PhysicalResourceNode::ReleaseParam;

        BTB();
        virtual ~BTB();

        void Initialize(InitPhase phase);

        // PCから次のPCを予測して返す
        BTBPredict Predict(const PC& pc);

        // BTBをupdateする
        void Update(const OpIterator& op, const BTBPredict& predict);
    };

}; // namespace Onikiri

#endif // __BTB_H__


