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
#include "Sim/Predictor/BPred/BTB.h"
#include "Sim/Foundation/SimPC.h"
#include "Sim/Op/Op.h"

using namespace Onikiri;
using namespace std;

BTB::BTB()
{
    m_numEntryBits  = 0;
    m_numWays       = 0;

    m_numPred       = 0;
    m_numTableHit   = 0;
    m_numTableMiss  = 0;
    m_numUpdate     = 0;
    m_numEntries    = 0;
    m_numHit        = 0;
    m_numMiss       = 0;
    m_table         = 0;
}

BTB::~BTB()
{
    if(m_table != 0)
        delete m_table;

    m_numEntries    = (1 << m_numEntryBits); // entry count
    ReleaseParam();
}

void BTB::Initialize(InitPhase phase)
{
    if(phase == INIT_PRE_CONNECTION){
        LoadParam();
        m_table = 
            new SetAssocTableType( HasherType(m_numEntryBits), m_numWays);
    }
}

// lookup BTB
BTBPredict BTB::Predict(const PC& pc)
{
    BTBPredict pred = {pc, pc, BT_NON ,false};
    ++m_numPred;

    // BTBPredict::hit is determined by a result of reading the table.
    // Cannot use a result of reading the table.
    pred.hit = m_table->read( pc.address, &pred ) != m_table->end();
    if( pred.hit ){
        m_numTableHit++;
    }
    else{
        m_numTableMiss++;
    }

    return pred;
}

// update BTB
void BTB::Update(const OpIterator& op, const BTBPredict& predict)
{
    if(!op->GetTaken())
        return;


    const OpClass& opClass = op->GetOpClass();
    bool conditinal = opClass.IsConditionalBranch();
    BranchTypeUtility util;

    // Branch result
    BTBPredict result;
    result.predIndexPC = predict.predIndexPC;
    result.hit         = true;
    result.target      = op->GetTakenPC();
    result.dirPredict  = conditinal;
    result.type        = util.OpClassToBranchType( opClass );

    ASSERT( result.type != BT_NON, "BTB must be updated by branch op." );

    //
    // Update table if 'op' is branch and branch is taken.
    // (Unconditional branch is always 'taken'.
    //
    m_table->write(result.predIndexPC.address, result);
    ++m_numUpdate;

    // 
    // BTB hit rate calculated from :
    // total : count of updating BTB
    // hit   : count of predicting correct target address
    //
    if(predict.hit && result.target == predict.target)
        ++m_numHit;
    else
        ++m_numMiss;
    
}
