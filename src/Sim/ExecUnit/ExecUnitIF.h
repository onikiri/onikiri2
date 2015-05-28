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


#ifndef SIM_EXEC_UNIT_EXEC_UNIT_IF_H
#define SIM_EXEC_UNIT_EXEC_UNIT_IF_H

#include "Sim/Op/OpArray/OpArray.h"

namespace Onikiri 
{
    class Scheduler;
    class ExecLatencyInfo;
    class ExecUnitReserver;
    class OpClass;

    // 演算器(ALUなど)のインターフェース
    class ExecUnitIF 
    {
    public:
        ExecUnitIF() {}
        virtual ~ExecUnitIF() {}


        // 実行レイテンシ後に FinishEvent を登録する
        virtual void Execute( OpIterator op ) = 0;

        // 毎サイクル呼ばれる
        virtual void Begin() = 0;

        // Called in Evaluate phase.
        virtual bool CanReserve( OpIterator op, int time ) = 0;
        
        // Called in Evaluate phase.
        virtual void Reserve( OpIterator op, int time ) = 0;

        // Called in Update phase.
        virtual void Update() = 0;

        // Get a reserver when you need to manually reserve execution units 
        // without CanReserve()/Reserve().
        virtual ExecUnitReserver* GetReserver() = 0; 
        
        // ExecLatencyInfoを返す
        virtual ExecLatencyInfo* GetExecLatencyInfo() = 0;

        // OpClass から取りうるレイテンシの種類の数を返す
        virtual int GetLatencyCount( const OpClass& opClass ) = 0;

        // OpClass とインデクスからレイテンシを返す
        virtual int GetLatency( const OpClass& opClass, int index ) = 0;

        // Return a code of OpClass mapped to this unit.
        virtual int GetMappedCode( int index ) = 0;
        virtual int GetMappedCodeCount() = 0;
    };

}; // namespace Onikiri

#endif // SIM_EXEC_UNIT_EXEC_UNIT_IF_H

