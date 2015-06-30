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

#include "Sim/Dependency/Dependency.h"
#include "Sim/Op/Op.h"

#include "Utility/RuntimeError.h"

using namespace Onikiri;
using namespace std;


Dependency::Dependency()
: m_readiness(0, false)
{
}

Dependency::Dependency(int numScheduler)
: m_readiness(numScheduler, false)
{
}

Dependency::~Dependency()
{
}

void Dependency::AddConsumer(OpIterator op)
{
    // Šù‚É consumer ‚Æ‚µ‚Ä“o˜^‚³‚ê‚Ä‚¢‚È‚¯‚ê‚Î“o˜^
    Dependency::ConsumerListType::iterator i = find(m_consumer.begin(), m_consumer.end(), op);
    if( i == m_consumer.end() ) {
        m_consumer.push_back(op);
    }
}

void Dependency::RemoveConsumer(OpIterator op)
{
    Dependency::ConsumerListType::iterator i = find(m_consumer.begin(), m_consumer.end(), op);
    if( i != m_consumer.end() ) {
        m_consumer.erase(i);
    }
}

void Dependency::Set()
{
    m_readiness.set();
}

void Dependency::Reset()
{
    m_readiness.reset();
}

void Dependency::Clear()
{
    m_readiness.reset();
    m_consumer.clear();
}
