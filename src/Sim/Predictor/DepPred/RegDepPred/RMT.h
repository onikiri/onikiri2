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


#ifndef __RMT_H__
#define __RMT_H__

#include "Sim/Foundation/Checkpoint/CheckpointedData.h"
#include "Interface/EmulatorIF.h"
#include "Sim/Dependency/PhyReg/PhyReg.h"
#include "Sim/Register/RegisterFile.h"
#include "Sim/Register/RegisterFreeList.h"
#include "Sim/Foundation/Hook/HookDecl.h"
#include "Sim/ISAInfo.h"

#include "Sim/Predictor/DepPred/RegDepPred/RegDepPredBase.h"

namespace Onikiri 
{

    // レジスタ依存関係予測器の実装
    // 物理レジスタの解放タイミングを正確に管理

    class RMT :
        public RegDepPredBase, public PhysicalResourceNode
    {
    protected:
        int m_numLogicalReg;
        int m_numRegSegment;

        //
        Core* m_core;
        CheckpointMaster* m_checkpointMaster;

        // emulator
        EmulatorIF* m_emulator;

        // レジスタ本体
        RegisterFile* m_registerFile;

        // 論理レジスタのセグメント情報を格納するテーブル
        boost::array<int, SimISAInfo::MAX_REG_COUNT> m_segmentTable;

        // フリーリスト
        // Allocationが可能な物理レジスタの番号を管理する
        RegisterFreeList* m_regFreeList;

        // 論理レジスタ番号をキーとした、物理レジスタのマッピングテーブル
        CheckpointedData<
            boost::array< int, SimISAInfo::MAX_REG_COUNT >
        > m_allocationTable;

        // opのデスティネーション・レジスタのコミット時に解放される物理レジスタのマッピングテーブル
        // opのデスティネーション・レジスタの物理レジスタ番号をキーとする
        std::vector<int> m_releaseTable;

        // member methods
        int GetRegisterSegmentID(int index)
        {
            return m_segmentTable[index];
        }

    public:
        // s_releaseRegHook 時のop は，解放されるレジスタの持ち主ではなく，
        // そのレジスタの解放をトリガした命令のop
        struct HookParam
        {   
            OpIterator op;
            int logicalRegNum;
            int physicalRegNum;
        };

    protected:
        // AllocateReg/ReleaseReg/DeallocateReg の実装
        virtual void AllocateRegBody( HookParam* param );
        virtual void ReleaseRegBody ( HookParam* param );
        virtual void DeallocateRegBody( HookParam* param ); 

    public:
        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( EmulatorIF, "emulator", m_emulator )
            RESOURCE_ENTRY( Core, "core", m_core )
            RESOURCE_ENTRY( RegisterFile, "registerFile", m_registerFile )
            RESOURCE_ENTRY( RegisterFreeList, "registerFreeList", m_regFreeList )
            RESOURCE_ENTRY( CheckpointMaster, "checkpointMaster", m_checkpointMaster )
        END_RESOURCE_MAP()

        RMT();
        virtual ~RMT();

        virtual void Initialize(InitPhase phase);

        // ソース・レジスタに対応する物理レジスタ番号を返す
        virtual int ResolveReg(const int lno);

        // ResolveReg と同様に物理レジスタ番号を返す．
        // ただし，現在の割り当て状態を観測するのみで，
        // 副作用がないことを保証する．
        virtual int PeekReg(const int lno) const;

        // デスティネーション・レジスタに物理レジスタ番号を割り当てる
        virtual int AllocateReg(OpIterator op, const int lno);

        // retireしたので、opが解放すべき物理レジスタを解放
        virtual void ReleaseReg(OpIterator op, const int lno, int phyRegNo);
        // flushされたので、opのデスティネーション・レジスタを解放
        virtual void DeallocateReg(OpIterator op, const int lno, int phyRegNo); 

        // num個物理レジスタを割り当てることができるかどうか
        virtual bool CanAllocate(OpIterator* infoArray, int numOp);

        // 論理/物理レジスタの個数
        virtual int GetRegSegmentCount();
        virtual int GetLogicalRegCount(int segment);
        virtual int GetTotalLogicalRegCount();

        //
        // Hook
        //

        // Prototype : void Method( HookParameter<RMT,HookParam>* param )
        static HookPoint<RMT,HookParam> s_allocateRegHook;
        static HookPoint<RMT,HookParam> s_releaseRegHook;
        static HookPoint<RMT,HookParam> s_deallocateRegHook;

    };

}; // namespace Onikiri

#endif // __RMT_H__

