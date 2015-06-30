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


#ifndef __EMULATORUTILITY_OPEMULATIONSTATE_H__
#define __EMULATORUTILITY_OPEMULATIONSTATE_H__

#include "Interface/OpStateIF.h"

#include "Interface/OpInfo.h"
#include "Emu/Utility/CommonOpInfo.h"

namespace Onikiri {
    namespace EmulatorUtility {

        class ProcessState;

        class OpEmulationState {
        private:
            OpStateIF* m_opState;
            OpInfo* m_opInfo;
            EmulatorUtility::ProcessState* m_processState;

            // 新しいアーキテクチャを追加したら調節する
            static const int MaxSrcCount = 8;   // SrcReg + Imm
            static const int MaxDstCount = 3;

            // ・即値は src に入れておく
            // ・ソースレジスタがゼロレジスタの場合は src に 0 を入れておく
            // ・デスティネーションレジスタがゼロレジスタの場合は，デスティネーションレジスタがないものとして扱い dst を OpState に反映させない
            boost::array<u64, MaxSrcCount> m_src;
            boost::array<u64, MaxDstCount> m_dst;

            u64 m_PC;
            u64 m_takenPC;
            bool m_taken;
            int m_pid;
            int m_tid;


        private:
            template<typename TOpInfo, typename TRegObtainer>
            void InitOperands(TRegObtainer obtainer)
            {
                TOpInfo* opInfo = static_cast<TOpInfo*>(m_opInfo);
                {
                    int immNum = opInfo->GetImmNum();
                    for (int i = 0; i < immNum; i ++) {
                        m_src[ opInfo->GetImmOpMap(i) ] = opInfo->GetImm(i);
                    }
                }

                {
                    int srcRegNum = opInfo->GetSrcRegNum();
                    for (int i = 0; i < srcRegNum; i ++) {
                        m_src[ opInfo->GetSrcRegOpMap(i) ] = obtainer(i);
                    }
                }
            }
        public:
            // レジスタの値をOpStateから取得する
            struct RegFromOpState : public std::unary_function<int, u64>
            {
                RegFromOpState(Onikiri::OpStateIF* opState) : m_opState(opState) {}

                u64 operator()(int index) {
                    return m_opState->GetSrc(index);
                }
                Onikiri::OpStateIF* m_opState;
            };

            // レジスタの値をregArrayから取得する
            template<typename TOpInfo>
            struct RegFromRegArray : public std::unary_function<int, u64>
            {
                RegFromRegArray(TOpInfo* opInfo, u64* regArray) : m_opInfo(opInfo), m_regArray(regArray)  {}

                u64 operator()(int index) {
                    return m_regArray[ m_opInfo->GetSrcReg(index) ];
                }
                TOpInfo* m_opInfo;
                u64* m_regArray;
            };

            // opInfoに従ってopStateから値を取得して初期化
            template<typename TOpInfo>
            OpEmulationState(Onikiri::OpStateIF* opState, TOpInfo* opInfo, EmulatorUtility::ProcessState* processState)
                : m_opState(opState), m_opInfo(opInfo), m_processState(processState)
            {
                Addr addr = m_opState->GetTakenPC();
                m_takenPC = addr.address;
                m_taken = false;
                m_pid = addr.pid;
                m_tid = addr.tid;

                m_PC = m_opState->GetPC().address;

                InitOperands<TOpInfo, RegFromOpState>( RegFromOpState(opState) );
            }

            // エミュレーション高速化用
            // レジスタ値を regArray から取得する
            template<typename TOpInfo>
            OpEmulationState(Onikiri::OpStateIF* opState, TOpInfo* opInfo, EmulatorUtility::ProcessState* processState, PC pc, u64 takenAddr, u64* regArray)
                : m_opState(opState), m_opInfo(opInfo), m_processState(processState)
            {
                m_takenPC = pc.address+4;
                m_taken = false;
                m_pid = pc.pid;
                m_tid = pc.tid;

                m_PC = pc.address;

                InitOperands<TOpInfo, RegFromRegArray<TOpInfo> >( RegFromRegArray<TOpInfo>(opInfo, regArray) );
            }

            // OpState に結果を反映させる
            template<typename TOpInfo>
            void ApplyEmulationState()
            {
                TOpInfo* opInfo = static_cast<TOpInfo*>(m_opInfo);
                m_opState->SetTaken(m_taken);
                m_opState->SetTakenPC( Addr(m_pid, m_tid, m_takenPC) );

                // opInfoに従って dst を設定
                int dstRegNum = opInfo->GetDstRegNum();
                for (int i = 0; i < dstRegNum; i ++) {
                    m_opState->SetDst( i, m_dst[ opInfo->GetDstRegOpMap(i) ]);
                }
            }

            // エミュレーション高速化用
            // regArray に結果を反映させる
            template<typename TOpInfo>
            void ApplyEmulationStateToRegArray(u64* regArray)
            {
                TOpInfo* opInfo = static_cast<TOpInfo*>(m_opInfo);

                // opInfoに従って dst を設定
                int dstRegNum = opInfo->GetDstRegNum();
                for (int i = 0; i < dstRegNum; i ++) {
                    regArray[ opInfo->GetDstReg(i) ] = m_dst[ opInfo->GetDstRegOpMap(i) ];
                }
            }

            void SetDst(int index, u64 value)
            {
                m_dst[index] = value;
            }
            u64 GetSrc(int index)
            {
                return m_src[index];
            }
            u64 GetDst(int index)
            {
                return m_dst[index];
            }

            //// Accessors for results
            void SetTaken(bool t)
            {
                m_taken = t;
            }
            bool GetTaken() const
            {
                return m_taken;
            }

            Onikiri::u64 GetPC() const
            {
                return m_PC;
            }

            void SetTakenPC(Onikiri::u64 pc)
            {
                m_takenPC = pc;
            }

            Onikiri::u64 GetTakenPC() const
            {
                return m_takenPC;
            }

            int GetPID() const
            {
                return m_pid;
            }

            int GetTID() const
            {
                return m_tid;
            }

            OpStateIF* GetOpState()
            {
                return m_opState;
            }
            OpInfo* GetOpInfo()
            {
                return m_opInfo;
            }
            EmulatorUtility::ProcessState* GetProcessState()
            {
                return m_processState;
            }
        };

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif
