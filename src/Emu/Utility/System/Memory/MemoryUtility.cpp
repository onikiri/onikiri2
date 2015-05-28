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
#include "Emu/Utility/System/Memory/MemoryUtility.h"
#include "SysDeps/Endian.h"

using namespace std;
using namespace Onikiri;
using namespace EmulatorUtility;

// Utility
u64 Onikiri::EmulatorUtility::TargetStrlen(MemorySystem* mem, u64 targetAddr)
{
#if 0
    
    BlockArray blocks;
    // とりあえずMapUnitSize バイトの範囲でstrlenを行う．普通はこれで足りる．
    SplitAtMapUnitBoundary(targetAddr, m_addrConv.GetMapUnitSize(), back_inserter(blocks) );

    u64 result = 0;
    for (BlockArray::iterator e = blocks.begin(); e != blocks.end(); ++e) {
        char* p = static_cast<char*>(m_addrConv.TargetToHost(e->addr));
        for (int i = 0; i < (int)e->size; i ++, p ++) {
            if (*p == '\0')
                return result;
            else
                result ++;
        }
    }

    // なんとMapUnitSizeバイトよりも長い文字列だったので，次のMapUnitSizeバイトに対してもstrlenを行う
    return result + TargetStrlen(targetAddr+m_addrConv.GetMapUnitSize());;
#else
    u64 length = 0;
    while(true){
        EmuMemAccess access( targetAddr + length, 1, false );
        mem->ReadMemory( &access );
        if( access.value == '\0' ){
            break;
        }
        length++;
    };

    return length;
#endif
}

// Get c string data from the targetAddr.
std::string Onikiri::EmulatorUtility::StrCpyToHost(MemorySystem* mem, u64 targetAddr) 
{
    std::string strData;
    u64 strAddr = targetAddr;
    while(true){
        EmuMemAccess access( strAddr, 1, false );
        mem->ReadMemory( &access );
        if( access.value == '\0' ){
            break;
        }
        strData += (char)access.value;
        strAddr++;
    };

    return strData;
}



// TargetBuffer
TargetBuffer::TargetBuffer(MemorySystem* memory, u64 targetAddr, size_t bufSize, bool readOnly)
    : m_memory(memory), m_targetAddr(targetAddr), m_bufSize(bufSize), m_readOnly(readOnly)
{
    // hostの連続領域にコピー
    m_buf = new u8[bufSize];
    m_memory->MemCopyToHost(m_buf, m_targetAddr, m_bufSize);
}

TargetBuffer::~TargetBuffer()
{
    // 書き戻す
    if (!m_readOnly)
        m_memory->MemCopyToTarget(m_targetAddr, m_buf, m_bufSize);
    delete[] m_buf;
}

void* TargetBuffer::Get()
{
    return m_buf;
}

const void* TargetBuffer::Get() const
{
    return m_buf;
}

