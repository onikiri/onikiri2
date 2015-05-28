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
#include "Sim/Foundation/Resource/ResourceBase.h"

using namespace Onikiri;

PhysicalResourceBase::PhysicalResourceBase()
{
    m_rid = RID_INVALID;
}

PhysicalResourceBase::~PhysicalResourceBase()
{
}

int  PhysicalResourceBase::GetThreadCount()
{
    return (int)m_tid.size();
}

void PhysicalResourceBase::SetThreadCount(const int count)
{
    m_tid.resize( count, TID_INVALID );
}

int  PhysicalResourceBase::GetTID(const int index)
{
    ASSERT( index < (int)m_tid.size() );
    return m_tid[index];
}

void PhysicalResourceBase::SetTID(const int index, const int tid)
{
    ASSERT( index < (int)m_tid.size() );
    m_tid[index] = tid;
}

int  PhysicalResourceBase::GetRID()
{
    return m_rid;
}

void PhysicalResourceBase::SetRID(const int rid)
{
    m_rid = rid;
}

//
// ---
//
LogicalResourceBase::LogicalResourceBase()
{
    m_pid = 0;
    m_tid = 0;
}

LogicalResourceBase::~LogicalResourceBase()
{
}

int  LogicalResourceBase::GetPID() const
{
    return m_pid;
}

void LogicalResourceBase::SetPID(const int pid)
{
    m_pid = pid;
}

int  LogicalResourceBase::GetTID() const
{
    return m_tid;
}

void LogicalResourceBase::SetTID(const int tid)
{
    m_tid = tid;
}

