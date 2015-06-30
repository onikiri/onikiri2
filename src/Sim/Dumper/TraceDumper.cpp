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

#include "Sim/Dumper/TraceDumper.h"

#include "Utility/RuntimeError.h"
#include "Sim/Dumper/DumpState.h"
#include "Sim/Dumper/DumpFileName.h"
#include "Sim/Op/Op.h"


using namespace std;
using namespace boost;
using namespace Onikiri;

TraceDumper::TraceDumper()
{
    m_enabled = false;
    m_detail = 0;
    m_valDetail = 0;
    m_cycle = 0;
    m_skipInsns = 0;
    m_flush = false;
}

TraceDumper::~TraceDumper()
{
}

void TraceDumper::Initialize( const String& suffix )
{
    LoadParam();
    Open( suffix );
}

void TraceDumper::Finalize()
{
    ReleaseParam();
    Close();
}

void TraceDumper::DumpString(const std::string& str)
{
    m_dumpStream << m_cycle << "\t" << str << "\n";
    if( m_flush )
        m_dumpStream.flush();
}

void TraceDumper::Open( const String& suffix )
{

    if( Enabled() && (!m_dumpStream.is_complete()) ) {
        String fileName = g_env.GetHostWorkPath() + MakeDumpFileName( m_filename, suffix, m_gzipEnabled );
        if (m_gzipEnabled){
            m_dumpStream.push( 
                iostreams::gzip_compressor( 
                    iostreams::gzip_params(m_gzipLevel) 
                )
            );
        }
        m_dumpStream.push( iostreams::file_sink(fileName, ios::binary));
    }
}

void TraceDumper::Close()
{
    if(m_dumpStream.is_complete()) {
        m_dumpStream.reset();
    }
}


void TraceDumper::Dump(DUMP_STATE state, Op* op, int detail)
{
    if( !Enabled() )
        return;

    if( op->GetGlobalSerialID() < m_skipInsns )
        return;

    const char* str = g_traceDumperStrTbl[state];

    if(detail == -1){
        if(op == NULL){
            DumpString(str);
        }
        else{
            Dump(state, op, Detail());
        }
    }
    else{
        ostringstream oss;
        oss << str << "\t" << op->ToString(detail, m_valDetail > 0);
        DumpString(oss.str());
    }

}

void TraceDumper::DumpStallBegin(Op* op)
{
    if( !Enabled() )
        return;
    
    if( op->GetGlobalSerialID() < m_skipInsns )
        return;

    ostringstream oss;
    oss << "stall begin" << "\t" << op->ToString(0);
    DumpString(oss.str());
}

void TraceDumper::DumpStallEnd(Op* op)
{
    if( !Enabled() )
        return;
    
    if( op->GetGlobalSerialID() < m_skipInsns )
        return;

    ostringstream oss;
    oss << "stall end" << "\t" << op->ToString(0);
    DumpString(oss.str());
}

void TraceDumper::SetCurrentCycle(s64 cycle)
{
    m_cycle = cycle;
}
