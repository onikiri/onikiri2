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

#include "Sim/Predictor/BPred/GlobalHistory.h"
#include "Utility/RuntimeError.h"

using namespace Onikiri;

GlobalHistory::GlobalHistory()  :
    m_checkpointMaster(0)
{
}

GlobalHistory::~GlobalHistory()
{
    ReleaseParam();
}

void GlobalHistory::Initialize(InitPhase phase)
{
    if(phase == INIT_POST_CONNECTION){
        CheckNodeInitialized( "checkpointMaster", m_checkpointMaster );

        m_globalHistory.Initialize(
            m_checkpointMaster,
            CheckpointMaster::SLOT_FETCH
        );
        *m_globalHistory = 0;
    }
}

// dirpred の予測時に予測結果を bpred から教えてもらう
void GlobalHistory::Predicted(bool taken)
{
    *m_globalHistory = ( (*m_globalHistory) << 1 ) | (taken ? 1 : 0);
}

// 分岐のRetire時にTaken/NotTakenを bpred から教えてもらう
// GlobalHistoryの更新は予測時/予測ミス発覚時に行うので、実装はしない
void GlobalHistory::Retired(bool taken)
{
}

// 最下位ビット(1番最新のもの)を(強制的に)変更する
// 分岐方向予測ミス時に正しい予測を学習するのに使う
void GlobalHistory::SetLeastSignificantBit(bool taken)
{
    *m_globalHistory = shttl::deposit(*m_globalHistory, 0, 1, taken);
}

// GlobalHistoryを返す
u64 GlobalHistory::GetHistory()
{
    return *m_globalHistory;
}
