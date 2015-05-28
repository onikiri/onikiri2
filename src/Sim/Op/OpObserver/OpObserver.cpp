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

#include "Sim/Op/OpObserver/OpObserver.h"

#include <algorithm>

using namespace std;
using namespace Onikiri;

// OpObserver

// OpObserver のデフォルトコンストラクタ
// OpNotifier を受け取らないので外側でOpNotifier::Addを呼ぶ
OpObserver::OpObserver()
{
}

OpObserver::OpObserver(OpNotifier* notifier)
{
    notifier->Add(this);
}

OpObserver::~OpObserver()
{
}


// OpNotifier
OpNotifier::OpNotifier()
: m_observer(0)
{
}

OpNotifier::~OpNotifier()
{
}

void OpNotifier::Add(OpObserver* observer)
{
    m_observer.push_back(observer);
}

void OpNotifier::NotifyCommit(OpIterator op)
{
    for_each(
        m_observer.begin(),
        m_observer.end(),
        NotifyFuncObj(op, &OpObserver::Commit)
    );
}

void OpNotifier::NotifyFlush(OpIterator op)
{
    for_each(
        m_observer.begin(),
        m_observer.end(),
        NotifyFuncObj(op, &OpObserver::Flush)
    );
}


