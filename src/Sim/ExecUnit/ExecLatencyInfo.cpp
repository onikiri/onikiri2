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

#include "Sim/ExecUnit/ExecLatencyInfo.h"
#include "Sim/Op/Op.h"
#include "Utility/RuntimeError.h"

using namespace Onikiri;

ExecLatencyInfo::ExecLatencyInfo()
{
}

ExecLatencyInfo::~ExecLatencyInfo()
{
    ReleaseParam();
}

void ExecLatencyInfo::Initialize(InitPhase phase)
{
    if(phase == INIT_PRE_CONNECTION){
        LoadParam();
        for(size_t i = 0; i < m_latencyInfo.size(); i++){
            int code    = OpClassCode::FromString(m_latencyInfo[i].code);
            int latency = m_latencyInfo[i].latency;
            if((int)m_latency.size() <= code)
                m_latency.resize(code+1, -1);
            m_latency[code] = latency;
        }
    }
}
/*
// system に初期化時にレイテンシを教えてもらう関数
void ExecLatencyInfo::SetLatency(int code, int latency)
{
    // code が最大のインデックスになるように拡張する
    while(code >= static_cast<int>(m_latency.size())) {
        m_latency.push_back(-1);
    }

    m_latency[code] = latency;
}
*/
int ExecLatencyInfo::GetLatency( int code ) const 
{
    // 範囲チェック
    ASSERT(code >= 0 && code < static_cast<int>(m_latency.size()), 
        "illegal code %d.", code);

    // レイテンシがセットされているかどうかのチェック
    ASSERT(m_latency[code] > 0, 
        "latency not set for code %d.", code);

    return m_latency[ code ];
}

int ExecLatencyInfo::GetLatency( OpIterator op ) const 
{
    int code = op->GetOpClass().GetCode();
    return GetLatency( code );
}
