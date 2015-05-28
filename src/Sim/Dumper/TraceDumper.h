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


#ifndef __TRACE_DUMPER_H__
#define __TRACE_DUMPER_H__

#include "Types.h"
#include "Env/Param/ParamExchange.h"
#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Dumper/DumpState.h"

namespace Onikiri {

    class TraceDumper : public ParamExchange
    {
    private:
        int m_enabled;
        int m_detail;
        int m_valDetail;

        bool m_gzipEnabled; // Dumpをgzip圧縮するかどうか
        int  m_gzipLevel;   // gzipの圧縮レベル
        bool m_flush;       // 出力毎にフラッシュを行うかどうか

        std::string m_filename;
        boost::iostreams::filtering_ostream m_dumpStream;
        
        s64 m_cycle;
        u64 m_skipInsns;

        void Open( const String& suffix );
        void Close();
        void DumpString(const std::string& str);
    public:
        TraceDumper();
        virtual ~TraceDumper();

        BEGIN_PARAM_MAP("/Session/Environment/Dumper/TraceDumper/")
            PARAM_ENTRY("@EnableDump",      m_enabled)
            PARAM_ENTRY("@DefaultDetail",   m_detail)
            PARAM_ENTRY("@DetailRegValue",  m_valDetail)
            PARAM_ENTRY("@FileName" ,       m_filename)
            PARAM_ENTRY("@EnableGzip",      m_gzipEnabled)
            PARAM_ENTRY("@GzipLevel",       m_gzipLevel)
            PARAM_ENTRY("@SkipInsns",       m_skipInsns)
            PARAM_ENTRY("@Flush",           m_flush)
        END_PARAM_MAP()

        bool Enabled() const { return m_enabled != 0; }
        int  Detail()  const { return m_detail;  }

        void Initialize( const String& suffix );
        void Finalize();
        void Dump(DUMP_STATE state, Op* op = NULL, int detail = -1);
        void DumpStallBegin(Op* op);
        void DumpStallEnd(Op* op);
        void SetCurrentCycle(s64 cycle);
    };


}; // namespace Onikiri

#endif // __DUMPER_H__

