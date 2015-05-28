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


//
// GlobalHistoryの基底クラス
//
// 分岐のグローバルな履歴を持ち、分岐予測時/分岐命令のRetire時にUpdateを行う
// このクラスでは、分岐予測時に投機的にUpdateをし、分岐ミス時にはCheckpointにより
// 自動的にGlobalHistoryの巻き戻しが行われる
//

#ifndef __GLOBALHISTORY_H__
#define __GLOBALHISTORY_H__

#include "Types.h"
#include "Sim/Foundation/Checkpoint/CheckpointedData.h"
#include "Sim/Foundation/Resource/ResourceNode.h"

namespace Onikiri 
{
    class CheckpointMaster;

    class GlobalHistory : public PhysicalResourceNode 
    {
    protected:
        CheckpointMaster* m_checkpointMaster;
        CheckpointedData<u64> m_globalHistory; // 分岐履歴: 1bitが分岐のTaken/NotTakenに対応

    public:
        GlobalHistory();
        virtual ~GlobalHistory();

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( CheckpointMaster, "checkpointMaster", m_checkpointMaster )
        END_RESOURCE_MAP()

        // 初期化
        void Initialize(InitPhase phase);

        // dirpred の予測時に予測結果を bpred から教えてもらう
        void Predicted(bool taken);
        // 分岐のRetire時にTaken/NotTakenを bpred から教えてもらう
        void Retired(bool taken);

        // 最下位ビット(1番最新のもの)を(強制的に)変更する
        void SetLeastSignificantBit(bool taken);

        // accessors
        u64 GetHistory();

    };

}; // namespace Onikiri

#endif // __GLOBALHISTORY_H__

