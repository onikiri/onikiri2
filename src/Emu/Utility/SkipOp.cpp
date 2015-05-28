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
#include "Emu/Utility/SkipOp.h"

// Read/Write ‚Ì‚ÝŽÀ‘•‚µ‚½Op

using namespace std;
using namespace Onikiri;

SkipOp::SkipOp(MemIF* mainMem) : m_mainMem(mainMem)
{
}

SkipOp::~SkipOp()
{
}

// OpStateIF
PC SkipOp::GetPC() const
{
    ASSERT(0, "do not call");
    return Addr();
}

const u64 SkipOp::GetSrc(const int index) const
{
    ASSERT(0, "do not call");
    return 0;
}

const u64 SkipOp::GetDst(const int index) const
{
    ASSERT(0, "do not call");
    return 0;
}


void SkipOp::SetDst(const int index, const u64 value)
{
    ASSERT(0, "do not call");
}

void SkipOp::SetTakenPC(const PC takenPC)
{
    ASSERT(0, "do not call");
}

PC SkipOp::GetTakenPC() const
{
    ASSERT(0, "do not call");
    return Addr();
}

void SkipOp::SetTaken(const bool taken)
{
    ASSERT(0, "do not call");
}

bool SkipOp::GetTaken() const
{
    ASSERT(0, "do not call");
    return false;
}

// MemIF
void SkipOp::Read( MemAccess* access )
{
    m_mainMem->Read(access);
    if( access->result != MemAccess::MAR_SUCCESS ){
        RUNTIME_WARNING( "An access violation occurs.\n%s", access->ToString().c_str() );
    }
}

void SkipOp::Write( MemAccess* access )
{
    m_mainMem->Write(access);
    if( access->result != MemAccess::MAR_SUCCESS ){
        RUNTIME_WARNING( "An access violation occurs.\n%s", access->ToString().c_str() );
    }
}
