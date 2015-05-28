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

#include "Sim/Memory/MemOrderManager/MemOrderManager.h"

#include "SysDeps/Endian.h"
#include "Utility/RuntimeError.h"
#include "Sim/Dumper/Dumper.h"

#include "Interface/OpClass.h"

#include "Sim/Foundation/Hook/HookUtil.h"
#include "Sim/InorderList/InorderList.h"
#include "Sim/Core/Core.h"
#include "Sim/Predictor/DepPred/MemDepPred/MemDepPredIF.h"
#include "Sim/Memory/Cache/Cache.h"
#include "Sim/Memory/Cache/CacheSystem.h"
#include "Sim/Thread/Thread.h"
#include "Sim/Recoverer/Recoverer.h"
#include "Sim/Pipeline/Fetcher/Fetcher.h"

#include "Sim/Op/Op.h"

namespace Onikiri
{
    MemOrderManager::MemImageAccessHook MemOrderManager::s_writeMemImageHook;
    MemOrderManager::MemImageAccessHook MemOrderManager::s_readMemImageHook;
}

using namespace std;
using namespace Onikiri;

MemOrderManager::MemOrderManager() :
    m_unified(false),
    m_unifiedCapacity(0),
    m_loadCapacity(0),
    m_storeCapacity(0),
    m_idealPartialLoad(false),
    m_removeOpsOnCommit(false),
    m_cacheLatency(0),
    m_numExecutedStoreForwarding(0),
    m_numRetiredStoreForwarding(0),
    m_numExecutedLoadAccess(0),
    m_numRetiredLoadAccess(0),
    m_cache(0),
    m_cacheSystem(0),
    m_core(0),
    m_emulator(0)
{
}

MemOrderManager::~MemOrderManager()
{
}

void MemOrderManager::Initialize( InitPhase phase )
{
    if( phase == INIT_PRE_CONNECTION ){
        LoadParam();
    }
    else if( phase == INIT_POST_CONNECTION ){

        // メンバ変数がセットされているかのチェック
        CheckNodeInitialized( "core", m_core );
        CheckNodeInitialized( "emulator", m_emulator );
        CheckNodeInitialized( "cacheSystem", m_cacheSystem );

        m_cache = m_cacheSystem->GetFirstLevelDataCache();
        m_cacheLatency = m_cache->GetStaticLatency();

        // Listなどの初期化
        m_loadList.resize(*m_core->GetOpArray());
        m_storeList.resize(*m_core->GetOpArray());
        m_memOperations.SetTargetEndian( m_emulator->GetISAInfo()->IsLittleEndian() );

        int fetchWidth = GetCore()->GetFetcher()->GetFetchWidth();
        if( m_unified ){
            if( fetchWidth > m_unifiedCapacity ){
                THROW_RUNTIME_ERROR( "The capacity of MemOrderManager is too small. It must be larger than fetch width." );
            }
        }
        else{
            if( fetchWidth > m_loadCapacity || fetchWidth > m_storeCapacity ){
                THROW_RUNTIME_ERROR( "The capacity of MemOrderManager is too small. It must be larger than fetch width." );
            }
        }
    }

}

void MemOrderManager::Finalize()
{
    ReleaseParam();
}

// Detecting access order violation between 
// a store executed in this cycle and 
// successor loads that are already executed on program order.
void MemOrderManager::DetectAccessOrderViolation( OpIterator store )
{
    /*
    例：
    プログラムオーダで上流側が上に書いてある

    store1   [A] = Y --- op
    load1    U   = [A]
    store2   [A] = Z
    load2    V   = [A]
    
    で store1 が再スケジューリングされたとき、load1, load2を再スケジューリングする
    また、load1がstore1よりも先に実行されていたときも、load1, load2を再スケジューリングする
    */

    // store でなければエラー
    ASSERT( store->GetOpClass().IsStore(), "Not store op." );

    // 知らない store であればエラー
    ASSERT( m_storeList.count(store) == 1, "unknown op." );

    OpIterator conflictedConsumer = GetConsumerLoad( store, store->GetMemAccess(), 0 );

    if( !conflictedConsumer.IsNull() ){
        
        if( store->GetSerialID() >= conflictedConsumer->GetSerialID() ) {
            THROW_RUNTIME_ERROR("memory order is not inorder.");
        }
        
        g_dumper.Dump( DS_ADDRESS_PREDICTION_MISS, conflictedConsumer );
        
        // MemDepPred に違反を通知
        store->GetThread()->GetMemDepPred()->OrderConflicted( store, conflictedConsumer );

        // 回復してもらう
        Recoverer* recoverer = store->GetThread()->GetRecoverer();
        recoverer->RecoverDataPredMiss( 
            store, 
            conflictedConsumer, 
            Recovery::TYPE_ADDRESS_MATCH 
        );

    }
}


// Detect and recover from partial read violation.
bool MemOrderManager::DetectPartialLoadViolation( OpIterator op )
{
    if( op->GetOpClass().IsLoad() ){
        // Validate whether invalid partial read occurs or not.
        // If invalid partial read occurs, MAR_READ_INVALID_PARTIAL_READ is 
        // set to a result and need to recover.
        const MemAccess& mem = op->GetMemAccess();
        if( mem.result == MemAccess::MAR_READ_INVALID_PARTIAL_READ ){
            OpIterator producer = 
                GetProducerStore( op, mem );
            Recoverer* recoverer = op->GetThread()->GetRecoverer();

            recoverer->RecoverDataPredMiss( 
                producer, 
                op, 
                DataPredMissRecovery::TYPE_PARTIAL_LOAD 
            );
            return true;
        }
    }
    return false;
}

bool MemOrderManager::CanAllocate( OpIterator* infoArray, int numOp )
{
    if( m_unified ){
        return m_unifiedCapacity >= (int)m_storeList.size() + (int)m_loadList.size() + numOp;
    }
    else{
        int loads  = 0;
        int stores = 0;
        for( int i = 0; i < numOp; ++i ){
            if( infoArray[i]->GetOpClass().IsStore() ){
                stores++;
            }
            if( infoArray[i]->GetOpClass().IsLoad() ){
                loads++;
            }
        }
        return 
            ( m_loadCapacity  >= ((int)m_loadList.size() + loads) ) &&
            ( m_storeCapacity >= ((int)m_storeList.size() + stores) );
    }
}

void MemOrderManager::Allocate(OpIterator op)
{
    const OpClass& opClass = op->GetOpClass();
    ASSERT( opClass.IsMem() );
    if( opClass.IsLoad() ){
        m_loadList.push_back( op );
    }
    else{
        m_storeList.push_back( op );
    }
}

// 実行終了
void MemOrderManager::Finished( OpIterator op )
{
    const OpClass& opClass = op->GetOpClass();
    // Detect access order violation.
    if( opClass.IsStore() ){
        ASSERT( m_storeList.count(op) == 1, "unknown op." );
        DetectAccessOrderViolation( op );
    }

    if( opClass.IsLoad() ){
        ASSERT( m_loadList.count(op) == 1, "unknown op." );
        DetectPartialLoadViolation( op );
    }
}

// アドレスが一致する先行のstoreがMemOrderManagerの中にいるかどうかを判断する
// InnerAccessのチェックは行わない
OpIterator MemOrderManager::GetProducerStore( OpIterator consumer, const MemAccess& access ) const
{
    ASSERT( access.address.pid != PID_INVALID );

    // アドレスが一致する先行のstoreがMemOrderManagerの中にいる場合、MemOrderManagerからReadを行う
    // m_listをopの位置から前方に探索
    ASSERT( m_loadList.count(consumer) == 1, "unknown load:\n%s", consumer->ToString().c_str() );

    OpIterator producer = OpIterator(0);

    OpList::const_iterator i = m_storeList.end();
    while( i != m_storeList.begin() ) {
        --i;

        OpIterator op = *i;

        if( op->GetGlobalSerialID() > consumer->GetGlobalSerialID() ){
            continue;
        }

        if( op->GetStatus() < OpStatus::OS_EXECUTING ){
            continue;
        }

        // エントリーに入っているOpがストアでアドレスが重なっていたら
        if( op->GetOpClass().IsStore() &&
            m_memOperations.IsOverlapped( access, op->GetMemAccess() ) 
        ){
            producer = op;
            break;
        }
    };

    return producer;
}

OpIterator MemOrderManager::GetConsumerLoad( OpIterator producer, const MemAccess& producerAccess, int index )
{
    ASSERT( producer->GetOpClass().IsStore(), "Not store op." );
    ASSERT( m_storeList.count( producer ) == 1, "unknown op." );

    OpIterator consumer;

    int num = 0;
    for( OpList::iterator i = m_loadList.begin(); i != m_loadList.end(); ++i ){
        
        OpIterator op = *i;
        if( op->GetGlobalSerialID() < producer->GetGlobalSerialID() ){
            continue;
        }

        if( op->GetStatus() < OpStatus::OS_EXECUTING ){
            continue;
        }

        OpClass opClass = op->GetOpClass();
        if( !opClass.IsLoad() ){
            continue;
        }

        if( m_memOperations.IsOverlapped( producerAccess, op->GetMemAccess() ) ){
            if( num == index ){
                consumer = op;
                break;
            }
            num++;
        }
    }

    return consumer;

}

void MemOrderManager::Commit(OpIterator op)
{
    // 幅の制限は外側で
    const OpClass& opClass = op->GetOpClass();
    if( opClass.IsStore() ){

        // エミュレータの持つメモリイメージへの読み書き
        MemAccess writeAccess = op->GetMemAccess();
        WriteMemImage( op, &writeAccess );

        if( writeAccess.result != MemAccess::MAR_SUCCESS ){
            RUNTIME_WARNING( 
                "An access violation occurs.\n%s\n%s",
                writeAccess.ToString().c_str(), op->ToString().c_str() 
            );
        }

        if( m_cache )   {
            CacheAccess access( op->GetMemAccess(), op, CacheAccess::OT_WRITE );
            CacheAccessResult result = 
                m_cache->Write( access, NULL ); // キャッシュに書き込む
            op->SetCacheAccessResult( result );
        }
    }

    if( opClass.IsLoad() ){
        const MemAccess& readAccess = op->GetMemAccess();
        if( readAccess.result != MemAccess::MAR_SUCCESS ){
            RUNTIME_WARNING( 
                "An access violation occurs.\n%s\n%s",
                readAccess.ToString().c_str(), op->ToString().c_str() 
            );
        }

        // Update dispersion of load accesses
        const CacheAccessResult& result = op->GetCacheAccessResult();
        if( result.cache == NULL ){ // NULL is store forwarding.
            m_numRetiredStoreForwarding++;
        }
        m_numRetiredLoadAccess++;
    }

    if( m_removeOpsOnCommit ){
        Delete( op );
    }
}

void MemOrderManager::Retire( OpIterator op )
{
    if( !m_removeOpsOnCommit ){
        Delete( op );
    }
}

void MemOrderManager::Flush( OpIterator op )
{
    Delete( op );
}

void MemOrderManager::Delete( OpIterator op )
{
    if( op->GetStatus() == OpStatus::OS_FETCH ){
        return;
    }

    const OpClass& opClass = op->GetOpClass();
    if( opClass.IsLoad() ) {
        m_loadList.erase(op);
    }
    else if( opClass.IsStore() ){
        m_storeList.erase(op);
    }

}

void MemOrderManager::Read( OpIterator op, MemAccess* access )
{
    OpIterator producer = GetProducerStore( op, *access );

    m_numExecutedLoadAccess++;

    if( !producer.IsNull() ){

        // アドレスが一致する先行のstoreがMemOrderManagerの中にいる場合、MemOrderManagerからReadを行う
        const MemAccess& producerAccess = producer->GetMemAccess();
        if( m_memOperations.IsInnerAccess( *access, producerAccess ) ){
            // Normal forwarding
            access->value = m_memOperations.ReadPreviousAccess( *access, producerAccess );
            m_numExecutedStoreForwarding++;
        }
        else{
            // Partial load forwarding
            if( !m_idealPartialLoad ){
                // Partial load is treated as exception.
                access->result = MemAccess::MAR_READ_INVALID_PARTIAL_READ;
            }
            else{
                // Ideal mode.
                // Store data is forwarded ideally.
                ReadMemImage( op, access );

                for( OpList::iterator i = m_storeList.begin(); i != m_storeList.end(); ++i ){
                    if( (*i)->GetGlobalSerialID() > op->GetGlobalSerialID() ){
                        break;
                    }
                    // Merge a store access to 'access'.
                    const MemAccess& storeAccess = (*i)->GetMemAccess();
                    if( m_memOperations.IsOverlapped( *access, storeAccess ) ){
                        access->value = 
                            m_memOperations.MergePartialAccess( *access, storeAccess );
                    }
                }
            }
        }
    }
    else{

        // アドレスが一致する先行のstoreが見つからなかったときは、emulator のもっているイメージから読む
        ReadMemImage( op, access );
    }

    op->SetMemAccess( *access );
}

void MemOrderManager::Write( OpIterator op, MemAccess* access )
{
    op->SetMemAccess( *access );
}

// エミュレータの持つメモリイメージへの読み書き
void MemOrderManager::ReadMemImage( OpIterator op, MemAccess* access )
{
    MemImageAccessParam param = { GetMemImage(), access };

    HOOK_SECTION_OP_PARAM( s_readMemImageHook, op, param )  
    {
        MemIF* memImage = param.memImage;
        memImage->Read( param.memAccess );
    }
}

void MemOrderManager::WriteMemImage( OpIterator op, MemAccess* access )
{
    MemImageAccessParam param = { GetMemImage(), access };

    HOOK_SECTION_OP_PARAM( s_writeMemImageHook, op, param ) 
    {
        MemIF* memImage = param.memImage;
        memImage->Write( param.memAccess );
    }
}
