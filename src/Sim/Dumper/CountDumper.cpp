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

#include "Sim/Dumper/CountDumper.h"
#include "Sim/Dumper/DumpFileName.h"

using namespace std;
using namespace boost;
using namespace Onikiri;

CountDumper::CountDumper()
{
    m_curInsnCount = 0;
    m_curCycleCount = 0;
    m_insnIntervalOrigin = 0;
    m_cycleIntervalOrigin = 0;
    m_nextUpdateInsnCount = 0;
}

CountDumper::~CountDumper()
{
}

void CountDumper::Initialize( const String& suffix )
{
    LoadParam();

    String fileName = g_env.GetHostWorkPath() + MakeDumpFileName( m_fileName, suffix, m_gzipEnabled );

    if( m_enabled && (!m_stream.is_complete()) ){
        if(m_gzipEnabled){
            m_stream.push(
                iostreams::gzip_compressor(
                    iostreams::gzip_params(m_gzipLevel) ) );
        }
        m_stream.push( 
            iostreams::file_sink(
                fileName, ios::binary) );
    }

    m_nextUpdateInsnCount = m_interval;
}

void CountDumper::Finalize()
{
    ReleaseParam();

    if(m_stream.is_complete()) {
        m_stream.reset();
    }
}

void CountDumper::SetCurrentInsnCount(s64 count)
{
    m_curInsnCount = count;
}

void CountDumper::SetCurrentCycle(s64 count)
{
    m_curCycleCount = count;
    Update();
}

bool CountDumper::Enabled()
{
    return m_enabled;
}

void CountDumper::Update()
{
    if(m_curInsnCount < m_nextUpdateInsnCount)
        return;

    s64 insnCountDelta  = m_curInsnCount  - m_insnIntervalOrigin;
    s64 cycleCountDelta = m_curCycleCount - m_cycleIntervalOrigin;
    double localIPC  = (double)insnCountDelta / (double)cycleCountDelta;
    double globalIPC = (double)m_curInsnCount / (double)m_curCycleCount;

    m_stream
        << "insns ,"        << insnCountDelta   << ","
        << "cycles ,"       << cycleCountDelta  << ","
        << "ipc ,"          << localIPC         << ","
        << "total insns ,"  << m_curInsnCount   << ","
        << "total cycles ," << m_curCycleCount  << ","
        << "total ipc ,"    << globalIPC        << ","
        << "\n";

    m_nextUpdateInsnCount += m_interval;
    m_insnIntervalOrigin  = m_curInsnCount;
    m_cycleIntervalOrigin = m_curCycleCount;
}
