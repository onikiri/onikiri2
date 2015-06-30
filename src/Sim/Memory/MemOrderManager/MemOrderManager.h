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


#ifndef SIM_MEMORY_MEM_ORDER_MANAGER_H
#define SIM_MEMORY_MEM_ORDER_MANAGER_H

#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Env/Param/ParamExchange.h"
#include "Sim/Foundation/Hook/Hook.h"

#include "Sim/Op/OpContainer/OpList.h"
#include "Sim/Op/OpContainer/OpExtraStateTable.h"

#include "Sim/Core/DataPredTypes.h"
#include "Sim/Memory/MemOrderManager/MemOrderOperations.h"

namespace Onikiri 
{
    class Core;
    class Cache;
    class CacheSystem;

    class MemOrderManager :
        public PhysicalResourceNode
    {

    public:
        BEGIN_PARAM_MAP("")
            
            BEGIN_PARAM_PATH( GetParamPath() )

                PARAM_ENTRY( "@Unified",         m_unified )
                PARAM_ENTRY( "@UnifiedCapacity", m_unifiedCapacity )
                PARAM_ENTRY( "@LoadCapacity",    m_loadCapacity )
                PARAM_ENTRY( "@StoreCapacity",   m_storeCapacity )
                PARAM_ENTRY( "@IdealPatialLoad", m_idealPartialLoad )
                PARAM_ENTRY( "@RemoveOpsOnCommit",  m_removeOpsOnCommit )
                
            END_PARAM_PATH()

            BEGIN_PARAM_PATH( GetResultPath() )

                BEGIN_PARAM_PATH( "ExecutedLoadAccess" )
                    PARAM_ENTRY( "@NumStoreForwarding", m_numExecutedStoreForwarding )
                    PARAM_ENTRY( "@NumLoadAccess",      m_numExecutedLoadAccess )
                END_PARAM_PATH()

                BEGIN_PARAM_PATH( "RetiredLoadAccess" )
                    PARAM_ENTRY( "@NumStoreForwarding", m_numRetiredStoreForwarding )
                    PARAM_ENTRY( "@NumLoadAccess",      m_numRetiredLoadAccess )
                END_PARAM_PATH()
            END_PARAM_PATH()

        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( EmulatorIF,  "emulator", m_emulator )
            RESOURCE_ENTRY( Core,        "core",        m_core )
            RESOURCE_ENTRY( CacheSystem, "cacheSystem", m_cacheSystem )
        END_RESOURCE_MAP()

        MemOrderManager();
        virtual ~MemOrderManager();

        virtual void Initialize( InitPhase phase );
        virtual void Finalize();

        // numOpだけAllocateできるかどうか
        bool CanAllocate( OpIterator* infoArray, int numOp );

        // fetchされたときにエントリを作る
        void Allocate( OpIterator op );
        
        // 実行終了
        void Finished( OpIterator op );

        // OpのaddressへのStore結果がMemOrderManagerのOpより前方にあるかどうか
        OpIterator GetProducerStore( OpIterator op, const MemAccess& access ) const;

        // Get a finished consumer of a producer store.
        // This method is used for detecting access order violation.
        // Returns OpIterator(0) if such consumers do not exist.
        // 'index' is an index of consumer in program order.
        OpIterator GetConsumerLoad( OpIterator producer, const MemAccess& access , int index );

        void Commit( OpIterator op );
        void Retire( OpIterator op );
        void Flush( OpIterator op );

        // op->Read, Write から呼ばれる Read, Write
        // op を引数で渡してもらう必要があるのでMemIFは継承しない
        void Read( OpIterator op, MemAccess* access );
        void Write( OpIterator op, MemAccess* access );

        // accessors
        MemIF* GetMemImage() const { return m_emulator->GetMemImage();  }
        Core*  GetCore()     const { return m_core;                     }
        bool   IsIdealPatialLoadEnabled() const { return m_idealPartialLoad; }

        // Hook
        struct MemImageAccessParam
        {
            MemIF*     memImage;
            MemAccess* memAccess;
        };
        typedef HookPoint<MemOrderManager, MemImageAccessParam> MemImageAccessHook;
        static MemImageAccessHook s_writeMemImageHook;
        static MemImageAccessHook s_readMemImageHook;

    protected:

        typedef DataPredMissRecovery Recovery;

        bool m_unified;             // Unified/dedicated load/store queue(s).
        int m_unifiedCapacity;      // MemOrderManagerが管理できるメモリ命令の最大数
        int m_loadCapacity;         // MemOrderManagerが管理できるメモリ命令の最大数
        int m_storeCapacity;        // MemOrderManagerが管理できるメモリ命令の最大数
        bool m_idealPartialLoad;    // Partial load is processed ideally or not.
        bool m_removeOpsOnCommit;   // If this parameter is true, ops are removed on commitment,
                                    // otherwise ops are removed on retirement.

        int m_cacheLatency; // 1次キャッシュのレイテンシ

        // statistic information
        s64 m_numExecutedStoreForwarding;   // Load命令の実行時にStoreBufferにデータがあった回数
        s64 m_numRetiredStoreForwarding;

        s64 m_numExecutedLoadAccess;        // StoreBufferへのアクセス回数
        s64 m_numRetiredLoadAccess;

        // L1D cache
        Cache* m_cache;
        CacheSystem* m_cacheSystem;

        // フェッチされてリタイアしてないロード・ストア命令列
        OpList m_loadList;
        OpList m_storeList;

        Core* m_core;
        EmulatorIF* m_emulator;

        // Operations related to detect address intersection.
        MemOrderOperations m_memOperations;

        // エミュレータが持つメモリイメージへの読み書き
        void ReadMemImage( OpIterator op, MemAccess* access );
        void WriteMemImage( OpIterator op, MemAccess* access );

        // Detect access order violation between 
        // a store executed in this cycle and 
        // already executed successor loads on program order.
        void DetectAccessOrderViolation( OpIterator store );

        // Detect and recover from partial read violation.
        // Returns true if violation is detected.
        bool DetectPartialLoadViolation( OpIterator op );

        void Delete( OpIterator op );

    };
}; // namespace Onikiri

#endif // SIM_MEMORY_MEM_ORDER_MANAGER_H

