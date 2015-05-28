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


#ifndef __COUNT_DUMPER_H
#define __COUNT_DUMPER_H

#include "Types.h"
#include "Env/Param/ParamExchange.h"
#include "Utility/RuntimeError.h"


namespace Onikiri 
{
    class CountDumper : public ParamExchange
    {
        bool m_enabled;
        bool m_gzipEnabled;
        int  m_gzipLevel;
        String m_fileName; 
        boost::iostreams::filtering_ostream m_stream;
        s64 m_interval;

        s64 m_nextUpdateInsnCount;
        s64 m_curInsnCount;
        s64 m_curCycleCount;
        s64 m_insnIntervalOrigin;
        s64 m_cycleIntervalOrigin;

        void Update();
    public:
        // parameter mapping
        BEGIN_PARAM_MAP("/Session/")
            BEGIN_PARAM_PATH("Environment/")
                PARAM_ENTRY("Dumper/CountDumper/@FileName",         m_fileName    ) 
                PARAM_ENTRY("Dumper/CountDumper/@EnableDump",       m_enabled     )
                PARAM_ENTRY("Dumper/CountDumper/@EnableGzip",       m_gzipEnabled )
                PARAM_ENTRY("Dumper/CountDumper/@GzipLevel",        m_gzipLevel   )
                PARAM_ENTRY("Dumper/CountDumper/@InsnCountInterval",    m_interval    )
            END_PARAM_PATH()
        END_PARAM_MAP()

        CountDumper();
        ~CountDumper();
        void Initialize( const String& suffix );
        void Finalize();

        void SetCurrentInsnCount( s64 count);
        void SetCurrentCycle(s64 count);
        bool Enabled();
    };

}; // namespace Onikiri

#endif // __COUNT_DUMPER_H

