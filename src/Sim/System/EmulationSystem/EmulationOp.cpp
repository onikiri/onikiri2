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
#include "Sim/System/EmulationSystem/EmulationOp.h"

using namespace std;
using namespace Onikiri;


EmulationOp::EmulationOp( MemIF* mainMem ) :
    m_opInfo ( NULL ),
    m_taken  ( false ),
    m_mem( mainMem )
{
}

EmulationOp::EmulationOp( const EmulationOp& op ) :
    m_pc     ( op.m_pc ),
    m_opInfo ( op.m_opInfo ),
    m_takenPC( op.m_takenPC ),
    m_taken  ( op.m_taken ),
    m_mem( op.m_mem ),
    m_dstReg ( op.m_dstReg ),
    m_srcReg ( op.m_srcReg )
{
}

EmulationOp::~EmulationOp()
{
}

void EmulationOp::SetPC( const PC& pc ) {
    m_pc = pc;
}

void EmulationOp::SetOpInfo( OpInfo* opInfo ) {
    m_opInfo = opInfo;
}

const OpInfo* EmulationOp::GetOpInfo() const
{
    return m_opInfo;
}

void EmulationOp::SetMem( MemIF* mem )
{
    m_mem = mem;
}

// OpStateIF
PC EmulationOp::GetPC() const
{
    return m_pc;
}

const u64 EmulationOp::GetSrc( const int index ) const
{
    return m_srcReg[index];
}

void EmulationOp::SetSrc( const int index, const u64 value )
{
    m_srcReg[index] = value;
}

const u64 EmulationOp::GetDst( const int index ) const
{
    return m_dstReg[index];
}


void EmulationOp::SetDst( const int index, const u64 value )
{
    m_dstReg[index] = value;
}

void EmulationOp::SetTakenPC( const PC takenPC )
{
    m_takenPC = takenPC;
}

PC EmulationOp::GetTakenPC() const
{
    return m_takenPC;
}

void EmulationOp::SetTaken( const bool taken )
{
    m_taken = taken;
}

bool EmulationOp::GetTaken() const
{
    return m_taken;
}

// MemIF
void EmulationOp::Read( MemAccess* access )
{
    ASSERT( m_mem != NULL );
    
    // Write correct 'tid', because an emulator does not initialized it.
    access->address.tid = m_pc.tid;
    
    m_mem->Read( access );
    if( access->result != MemAccess::MAR_SUCCESS ){
        RUNTIME_WARNING( "An access violation occurs.\n%s", access->ToString().c_str() );
    }
}

void EmulationOp::Write( MemAccess* access )
{
    ASSERT( m_mem != NULL );

    // Write correct 'tid', because an emulator does not initialized it.
    access->address.tid = m_pc.tid;

    m_mem->Write( access );
    if( access->result != MemAccess::MAR_SUCCESS ){
        RUNTIME_WARNING( "An access violation occurs.\n%s", access->ToString().c_str() );
    }
}
