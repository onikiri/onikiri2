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

#include "Sim/Predictor/DepPred/MemDepPred/MemDepPred.h"

using namespace Onikiri;



MemDepPred::MemDepPred() :
    m_pred( NULL )
{

}

MemDepPred::~MemDepPred()
{
    ReleaseParam();
}

void MemDepPred::Initialize( InitPhase phase )
{
    // Do not call "Initialize()" of a proxy target object,
    // because the "Initialize()" is called by the resource system.

    if( phase == INIT_PRE_CONNECTION ){
        LoadParam();
    }
    else if( phase == INIT_POST_CONNECTION ){
        CheckNodeInitialized( "memDepPred", m_pred );
    }
}

void MemDepPred::Resolve( OpIterator op )
{
    m_pred->Resolve( op );
}

void MemDepPred::Allocate( OpIterator op )
{
    m_pred->Allocate( op );
}

void MemDepPred::Commit( OpIterator op )
{
    m_pred->Commit( op );
}

void MemDepPred::Flush( OpIterator op )
{
    m_pred->Flush( op );
}

void MemDepPred::OrderConflicted( OpIterator producer, OpIterator consumer )
{
    m_pred->OrderConflicted( producer, consumer );
}

bool MemDepPred::CanAllocate( OpIterator* infoArray, int numOp )
{
    return m_pred->CanAllocate( infoArray, numOp );
}
