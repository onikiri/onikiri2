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


#ifndef EMU_UTILITY_COMMON_EMULATOR_H
#define EMU_UTILITY_COMMON_EMULATOR_H

#include "Emu/Utility/GenericOperation.h"
#include "Interface/EmulatorIF.h"
#include "Interface/OpStateIF.h"
#include "Interface/OpClassCode.h"
#include "Env/Param/ParamExchange.h"
#include "SysDeps/Endian.h"
#include "Emu/Utility/System/Syscall/SyscallConvIF.h"
#include "Emu/Utility/System/ProcessState.h"
#include "Emu/Utility/System/Memory/MemorySystem.h"
#include "Emu/Utility/System/Memory/MemoryUtility.h"
#include "Emu/Utility/System/VirtualSystem.h"
#include "Emu/Utility/ExtraOpInfoWrapper.h"
#include "Emu/Utility/OpEmulationState.h"
#include "Emu/Utility/SkipOp.h"
#include "Emu/Utility/GenericOperation.h"

namespace Onikiri {
    class SystemIF;

    namespace EmulatorUtility {

        // エミュレータ
        template <class Traits>
        class CommonEmulator : 
            public EmulatorIF,
            public MemIF, 
            public ParamExchange
        {
        private:
            // Traits は，これらの型をtypedefした構造体
            // これらの型を各システムに特化した型に定義することで，各システム専用のエミュレータを定義できる
            typedef typename Traits::ISAInfoType ISAInfoType;
            typedef typename Traits::OpInfoType OpInfoType;
            typedef typename Traits::ConverterType ConverterType;
            typedef typename Traits::LoaderType LoaderType;
            typedef typename Traits::SyscallConvType SyscallConvType;

            typedef ParamExchange ParamExchangeType;
        public:
            CommonEmulator( SystemIF* simSystem );
            virtual ~CommonEmulator();

            // ParamExchange 中の関数を宣言
            using ParamExchangeType::LoadParam;
            using ParamExchangeType::ReleaseParam;
            using ParamExchangeType::GetRootPath;
            using ParamExchangeType::ParamEntry;

            // EmulatorIF の実装
            virtual std::pair<OpInfo**, int> GetOp(PC pc);
            virtual MemIF* GetMemImage();
            virtual void Execute(OpStateIF* op, OpInfo* opinfo);
            virtual void Commit(OpStateIF* op, OpInfo* opinfo);
            virtual int GetProcessCount() const;
            virtual PC GetEntryPoint(int pid) const;
            virtual u64 GetInitialRegValue(int pid, int index) const;
            virtual ISAInfoIF* GetISAInfo();
            virtual PC Skip(PC pc, u64 skipCount, u64* regArray, u64* executedInsnCount, u64* executedOpCount);
            virtual void SetExtraOpDecoder( ExtraOpDecoderIF* extraOpDecoder );

            // MemIF の実装
            virtual void Read( MemAccess* access );
            virtual void Write( MemAccess* access );

        private:
            bool CreateProcesses( SystemIF* simSystem );
            
            // 高速化のための非バーチャルな実装
            std::pair<OpInfo**, int> GetOpBody(PC pc);

            ISAInfoType m_alpha64Info;
            std::vector<EmulatorUtility::ProcessState*> m_processes;
            ConverterType m_insnConverter;

            // 変換した結果の OpInfo をキャッシュ
            boost::pool<> m_opInfoArrayPool;
            boost::object_pool< OpInfoType > m_opInfoPool;
            boost::object_pool< ExtraOpInfoWrapper<ISAInfoType> > m_exOpInfoWrapperPool;

            class CodeWordHash
            {
            public:
                size_t operator()(const u32 value) const
                {
                    return (size_t)((value >> 26) ^ value);
                }
            };
            // アドレスから，そのアドレスの命令に対応するOpInfoを得る
            typedef std::pair<OpInfo**, int> OpInfoArray;
            std::vector< std::vector< OpInfoArray > > m_ProcessOpInfoCache;
            // 命令語から，opInfoArray 中の対応するOpInfoのポインタへのポインタと OpInfo の数を得る
            typedef unordered_map<u32, OpInfoArray, CodeWordHash> CodeWordOpInfoMap;
            CodeWordOpInfoMap m_codeWordToOpInfo;

            // param_map
            int m_processCount;
            
            // 外部命令デコーダ
            ExtraOpDecoderIF* m_extraOpDecoder;

            // CRCs of PCs and results of all retired ops.
            bool m_enableResultCRC;
            typedef typename boost::crc_32_type CRC_Type;
            std::vector< CRC_Type > m_resultCRCs;
            void CalcCRC( const PC& pc, u64 dstResult );
            std::vector< u32 > GetCRC_Result()
            {
                std::vector< u32 > result;
                for( std::vector< CRC_Type >::iterator i = m_resultCRCs.begin();
                     i != m_resultCRCs.end();
                     ++i
                ){
                    result.push_back( (*i)() );
                }
                return result;
            }

        public:

            BEGIN_PARAM_MAP( "" )
                BEGIN_PARAM_PATH( "/Session/Emulator/" )
                    PARAM_ENTRY( "@EnableResultCRC32Calculation", m_enableResultCRC )
                    BEGIN_PARAM_PATH("Processes/")
                        PARAM_ENTRY("count(Process)", m_processCount)
                    END_PARAM_PATH()
                END_PARAM_PATH()
                RESULT_ENTRY( "/Session/Result/System/@ResultCRC32", GetCRC_Result() )
            END_PARAM_MAP()
        };

        template <class Traits>
        CommonEmulator<Traits>::CommonEmulator( SystemIF* simSystem )
            :   m_opInfoArrayPool(sizeof(OpInfo*)),
                m_extraOpDecoder(0),
                m_enableResultCRC( false )
        {
            // param, プロセス情報読み込み
            LoadParam();
            if (!CreateProcesses(simSystem)) {
                THROW_RUNTIME_ERROR("failed to create target processes.");
            }
            Operation::testroundmode(NULL);
        }

        template <class Traits>
        CommonEmulator<Traits>::~CommonEmulator()
        {
            for (std::vector<ProcessState*>::iterator e = m_processes.begin(); e != m_processes.end(); ++e) {
                delete *e;
            }

            ReleaseParam();
        }

        template <class Traits>
        ISAInfoIF* CommonEmulator<Traits>::GetISAInfo()
        {
            return &m_alpha64Info;
        }

        template <class Traits>
        bool CommonEmulator<Traits>::CreateProcesses( SystemIF* simSystem )
        {
            ASSERT(m_processes.empty(), "CommonEmulator<Traits>::CreateProcesses called twice.");

            m_ProcessOpInfoCache.resize( m_processCount );
            m_resultCRCs.resize( m_processCount );

            for (int i = 0; i < m_processCount; i++) {
                ProcessCreateParam createParam(i);
                ProcessState* processState = new ProcessState(i);
                processState->Init<Traits>( createParam, simSystem );
                m_processes.push_back(processState);
                m_ProcessOpInfoCache[i].resize( processState->GetCodeRange().second, OpInfoArray((OpInfo**)NULL, 0));
            }
            
            return true;
        }

        // 外部命令デコーダをセットする
        template <class Traits>
        void CommonEmulator<Traits>::SetExtraOpDecoder( ExtraOpDecoderIF* extraOpDecoder ) 
        {
            m_extraOpDecoder = extraOpDecoder;
        }

        template <class Traits>
        int CommonEmulator<Traits>::GetProcessCount() const
        {
            return m_processCount;
        }

        template <class Traits>
        PC CommonEmulator<Traits>::GetEntryPoint(int pid) const
        {
            ASSERT((size_t)pid < m_processes.size());
            return PC(pid, TID_INVALID, m_processes[pid]->GetEntryPoint() );
        }

        template <class Traits>
        u64 CommonEmulator<Traits>::GetInitialRegValue(int pid, int index) const
        {
            assert((size_t)pid < m_processes.size());
            return m_processes[pid]->GetInitialRegValue(index);
        }

        template <class Traits>
        std::pair<OpInfo**, int> CommonEmulator<Traits>::GetOpBody(PC pc)
        {
            ProcessState* process = m_processes[pc.pid];

            // プロセスごとのキャッシュをまず見る
            const std::pair<u64, size_t> codeRange = process->GetCodeRange();
            // 短絡的評価を無効にし分岐を減らすために && ではなく & を使用
            bool withinCodeRange = (codeRange.first <= pc.address) & (pc.address < codeRange.first+codeRange.second);

            OpInfoArray* processOpInfoCacheEntry = NULL;
            if (withinCodeRange) {
                processOpInfoCacheEntry = &m_ProcessOpInfoCache[pc.pid][static_cast<size_t>(pc.address-codeRange.first)];
                if (processOpInfoCacheEntry->first != NULL)
                    return *processOpInfoCacheEntry;
            }

            // 命令語を取得
            EmuMemAccess codeAccess( pc.address, 4 );
            process->GetMemorySystem()->ReadMemory( &codeAccess );
            u32 codeWord = (u32)codeAccess.value;

            // 命令語をキーとしたキャッシュを見る
            typename CodeWordOpInfoMap::iterator e = m_codeWordToOpInfo.find(codeWord);
            if (e != m_codeWordToOpInfo.end()) {
                if (processOpInfoCacheEntry)
                    *processOpInfoCacheEntry = e->second;
                return e->second;
            }
            else {
                // 未変換ならば変換を行う
                std::pair<ExtraOpInfoIF**, int> decodedExOps;
                std::vector<OpInfoType> tempOpInfo;
                OpInfo** opInfoArray;
                int nOps;

                if( m_extraOpDecoder &&
                    m_extraOpDecoder->Decode( codeWord, &decodedExOps ) 
                ){
                    // 外部デコーダで命令デコードが行われた場合，
                    // ExtraOpInfoWrapper にデコード結果を格納

                    // 変換結果を格納するポインタの配列を opInfoArrayPool に確保する
                    nOps = decodedExOps.second;
                    opInfoArray = (OpInfo**)m_opInfoArrayPool.ordered_malloc( nOps );
                    for( int i = 0; i < nOps; i++ ){
                        ExtraOpInfoWrapper<ISAInfoType>* wrapper = m_exOpInfoWrapperPool.construct();
                        wrapper->SetExtraOpInfo( decodedExOps.first[i] );
                        opInfoArray[i] = wrapper;
                    }
                }
                else{
                    m_insnConverter.Convert(codeWord, back_inserter(tempOpInfo));

                    // 変換結果を格納するポインタの配列を opInfoArrayPool に確保する
                    nOps = (int)tempOpInfo.size();
                    opInfoArray = (OpInfo**)m_opInfoArrayPool.ordered_malloc(nOps);
                    for (int i = 0; i < nOps; i ++) {
                        // opInfoPool に 変換結果の opInfo のコピーを作成し，opInfoArray に格納
                        opInfoArray[i] = m_opInfoPool.construct(tempOpInfo[i]);
                    }
                }


                // キャッシュに格納して結果を返す
                OpInfoArray result(opInfoArray, nOps);
                m_codeWordToOpInfo[codeWord] = result;
                if (processOpInfoCacheEntry)
                    *processOpInfoCacheEntry = result;

                return result;
            }
        }

        template <class Traits>
        std::pair<OpInfo**, int> CommonEmulator<Traits>::GetOp(PC pc)
        {
            return GetOpBody(pc);
        }

        template <class Traits>
        MemIF* CommonEmulator<Traits>::GetMemImage()
        {
            return this;
        }

        template <class Traits>
        void CommonEmulator<Traits>::Execute(OpStateIF* op, OpInfo* opInfoBase)
        {
            OpInfoType* opInfo = static_cast<OpInfoType*>(opInfoBase);
            int pid = op->GetPC().pid;
            ProcessState* process = m_processes[pid];

            // 命令を実行する
            OpEmulationState opState(op, opInfo, process);  // opからオペランド等を読んで OpEmulationState を構築
            opInfo->GetEmulationFunc()(&opState);
            opState.ApplyEmulationState<OpInfoType>();  // op に値を設定
            process->GetVirtualSystem()->AddInsnTick();
        }

        template <class Traits>
        void CommonEmulator<Traits>::Commit( OpStateIF* op, OpInfo* info )
        {
            if( m_enableResultCRC ){
                u64 dst = info->GetDstNum() > 0 ? op->GetDst(0) : 0;
                CalcCRC( op->GetPC(), dst );
            }
        }

        template <class Traits>
        void CommonEmulator<Traits>::CalcCRC( const PC& pc, u64 dstResult )
        {
            m_resultCRCs[ pc.pid ].process_bytes( &pc.address , sizeof(pc.address) );
            m_resultCRCs[ pc.pid ].process_bytes( &dstResult , sizeof(dstResult) );
        }

        template <class Traits>
        PC CommonEmulator<Traits>::Skip(PC pc, u64 skipCount, u64* regArray, u64* executedInsnCount, u64* executedOpCount)
        {
            if (skipCount == 0) {
                if (executedInsnCount)
                    *executedInsnCount = 0;
                if (executedOpCount)
                    *executedOpCount = 0;
                return pc;
            }

            u64 totalOpCount = 0;
            u64 initialSkipCount = skipCount;
            ProcessState* process = m_processes[pc.pid];
            VirtualSystem* virtualSystem = process->GetVirtualSystem();
            bool enableResultCRC = m_enableResultCRC;

            SkipOp op(this);
            while (skipCount-- != 0 && pc.address != 0) {
                std::pair<OpInfo**, int> ops_pair = GetOpBody(pc);
                OpInfo** opInfoArray = ops_pair.first;
                int opCount = ops_pair.second;

                // opInfoArray の命令を全て実行
                for (int opIndex = 0; opIndex < opCount; opIndex ++) {
                    totalOpCount++;
                    OpInfoType* opInfo = static_cast<OpInfoType*>( opInfoArray[opIndex] );
                    
                    OpEmulationState opState(&op, opInfo, process, pc, pc.address + ISAInfoType::InstructionWordBitSize/8, regArray);   // opからオペランド等を読んで OpEmulationState を構築
                    opInfo->GetEmulationFunc()(&opState);
                    opState.ApplyEmulationStateToRegArray<OpInfoType>(regArray);

                    if( enableResultCRC ){
                        u64 dst = opInfo->GetDstNum() > 0 ? opState.GetDst(0) : 0;
                        CalcCRC( pc, dst );
                    }
                    virtualSystem->AddInsnTick();
    
                    if (opIndex == opCount-1) {
                        if (opState.GetTaken())
                            pc.address = opState.GetTakenPC();
                        else
                            pc.address += ISAInfoType::InstructionWordBitSize/8;
                        break;
                    }
                }
            }

            if (executedInsnCount)
                *executedInsnCount = initialSkipCount - skipCount;
            if (executedOpCount)
                *executedOpCount = totalOpCount;
            return pc;
        }

        template <class Traits>
        void CommonEmulator<Traits>::Read( MemAccess* access )
        {
            int pid = access->address.pid;
            ProcessState* process = m_processes[pid];
            process->GetMemorySystem()->ReadMemory(access);
        }

        template <class Traits>
        void CommonEmulator<Traits>::Write( MemAccess* access )
        {
            int pid = access->address.pid;
            ProcessState* process = m_processes[pid];
            process->GetMemorySystem()->WriteMemory(access);
        }

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif

