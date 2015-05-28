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


#ifndef SIM_LATENCY_EXEC_LATENCY_INFO_H
#define SIM_LATENCY_EXEC_LATENCY_INFO_H

#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Env/Param/ParamExchange.h"
#include "Sim/Op/OpArray/OpArray.h"

namespace Onikiri
{

    // 命令のコードに対応するレイテンシを返すクラス
    // 命令のコードがシステム固有なので、外からデータをセットしてもらう
    class ExecLatencyInfo : 
        public PhysicalResourceNode
    {
    public:
        
        BEGIN_PARAM_MAP( GetParamPath() )
            PARAM_ENTRY("@Name", m_name)
            CHAIN_PARAM_MAP("Latency", m_latencyInfo)
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
        END_RESOURCE_MAP()

        ExecLatencyInfo();
        virtual ~ExecLatencyInfo();
        void Initialize(InitPhase phase);

    //  void SetLatency(int code, int latency);
        int GetLatency( int code ) const;
        int GetLatency( OpIterator op ) const;

    protected:
        std::vector< int > m_latency;
        String m_name;

        struct LatencyInfo : public ParamExchangeChild
        {
            String code;
            int    latency;
            BEGIN_PARAM_MAP( "" )
                PARAM_ENTRY( "@Code",    code )
                PARAM_ENTRY( "@Latency", latency )
            END_PARAM_MAP()
        };
        
        std::vector<LatencyInfo> m_latencyInfo;
    };

}; // namespace Onikiri

#endif // __EXECLATENCYINFO_H__

