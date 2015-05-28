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


#ifndef EMU_UTILITY_COMMON_OP_INFO_H
#define EMU_UTILITY_COMMON_OP_INFO_H

#include "Utility/RuntimeError.h"
#include "Interface/OpInfo.h"
#include "Interface/OpStateIF.h"
#include "Interface/ExtraOpDecoderIF.h"
#include "Interface/OpClass.h"


namespace Onikiri 
{
    namespace EmulatorUtility 
    {

        class OpEmulationState;


        template<class TISAInfo>
        class CommonOpInfo : public OpInfo
        {
        public:
            static const int MaxSrcRegCount = TISAInfo::MaxSrcRegCount;
            static const int MaxDstRegCount = TISAInfo::MaxDstRegCount;
            static const int MaxImmCount = TISAInfo::MaxImmCount;
            static const int MaxSrcCount = MaxSrcRegCount + MaxImmCount;
            static const int MaxDstCount = MaxDstRegCount;
            enum OperandType { NONE, REG, IMM, ZERO };  // ZERO は dst専用
            typedef void (*EmulationFunc)(OpEmulationState*);

            explicit CommonOpInfo(OpClass opClass) : 
                m_opClass(opClass),
                m_emulationFunc(0), 
                m_srcRegNum(0), 
                m_dstRegNum(0), 
                m_srcOpNum(0), 
                m_dstOpNum(0),
                m_immNum(0), 
                m_microOpNum(0), 
                m_microOpIndex(0), 
                m_mnemonic("")
            {
                std::fill(m_srcReg.begin(), m_srcReg.end(), -1);
                std::fill(m_dstReg.begin(), m_dstReg.end(), -1);
                std::fill(m_imm.begin(), m_imm.end(), -1);

                std::fill(m_srcRegOpMap.begin(), m_srcRegOpMap.end(), -1);
                std::fill(m_dstRegOpMap.begin(), m_dstRegOpMap.end(), -1);
                std::fill(m_immOpMap.begin(), m_immOpMap.end(), -1);
            }
            virtual ~CommonOpInfo()
            {
            }

            int GetSrcReg(int index) { return m_srcReg[index]; }
            void SetSrcReg(int index, int value) 
            {
                assert( index < MaxSrcRegCount );   // For avoiding gcc's warning.
                m_srcReg[index] = value; 
            }

            int GetDstReg(int index) { return m_dstReg[index]; }
            void SetDstReg(int index, int value) 
            {
                assert( index < MaxDstRegCount );   // For avoiding gcc's warning.
                m_dstReg[index] = value; 
            }

            s64 GetImm(int index) { return m_imm[index]; }
            void SetImm(int index, s64 value)
            {
                assert( index < MaxImmCount );  // For avoiding gcc's warning.
                m_imm[index] = value; 
            }

            EmulationFunc GetEmulationFunc() const { return m_emulationFunc; }
            void SetEmulationFunc(EmulationFunc func) { m_emulationFunc = func; }

            // 対応 
            void SetSrcRegOpMap(int index, int value)
            {
                assert( index < MaxSrcCount );  // For avoiding gcc's warning.
                m_srcRegOpMap[index] = value; 
            }
            int GetSrcRegOpMap(int index) const { return m_srcRegOpMap[index]; }

            void SetImmOpMap(int index, int value) 
            {
                assert( index < MaxImmCount );  // For avoiding gcc's warning.
                m_immOpMap[index] = value; 
            }
            int GetImmOpMap(int index) const { return m_immOpMap[index]; }

            void SetDstRegOpMap(int index, int value) 
            {
                assert( index < MaxDstCount );  // For avoiding gcc's warning.
                m_dstRegOpMap[index] = value; 
            }
            int GetDstRegOpMap(int index) const { return m_dstRegOpMap[index]; }

            // ソース・デスティネーション・オペランドの個数を設定
            void SetSrcOpNum(int n) { m_srcOpNum = n; }
            int GetSrcOpNum() const { return m_srcOpNum; }
            void SetDstOpNum(int n) { m_dstOpNum = n; }
            int GetDstOpNum() const { return m_dstOpNum; }

            // 即値の個数を設定
            void SetImmNum(int n) { m_immNum = n; }
            int GetImmNum() const { return m_immNum; }

            // ソース・デスティネーション「レジスタ」の個数を設定
            void SetSrcRegNum(int n) { m_srcRegNum = n; }
            void SetDstRegNum(int n) { m_dstRegNum = n; }

            int GetSrcRegNum() const { return m_srcRegNum; }    // non-virtual な GetSrcNum
            int GetDstRegNum() const { return m_dstRegNum; }

            // The number of micro-ops for its instruction.
            void SetMicroOpNum( int microOpNum )    {   m_microOpNum = microOpNum;      }
            void SetMicroOpIndex( int microOpIndex ){   m_microOpIndex = microOpIndex;  }

            // ニーモニックを設定 (ポインタは一時的な文字列を指していてはいけない)
            void SetMnemonic(const char* mnemonic) { m_mnemonic = mnemonic; }

            // --- Implementation of OpInfo
            virtual const OpClass& GetOpClass() const
            {
                return m_opClass;
            }
            virtual int GetSrcOperand(const int index) const
            {
                assert(index < MaxSrcRegCount);
                return m_srcReg[index];
            }
            virtual int GetDstOperand(const int index) const
            {
                assert(index < MaxDstRegCount);
                return m_dstReg[index];
            }
            virtual int GetSrcNum() const
            {
                return m_srcRegNum;
            }
            virtual int GetDstNum() const
            {
                return m_dstRegNum;
            }

            // The number of micro-ops for its instruction.
            virtual int GetMicroOpNum() const
            {
                return m_microOpNum;
            }

            // The position of this micro op in its instruction.
            virtual int GetMicroOpIndex() const
            {
                return m_microOpIndex;
            }

            virtual const char* GetMnemonic() const
            {
                return m_mnemonic;
            }

        protected:
            OpClass m_opClass;
            
            // 命令に対応する操作を実行する関数
            EmulationFunc m_emulationFunc;
            // ソースレジスタ番号
            boost::array<int, MaxSrcRegCount> m_srcReg;
            // デスティネーションレジスタ番号
            boost::array<int, MaxDstRegCount> m_dstReg;
            // 即値
            boost::array<s64, MaxImmCount>    m_imm;
            // srcRegの数
            int m_srcRegNum;
            // dstRegの数
            int m_dstRegNum;

            // srcの数
            int m_srcOpNum;
            // dstの数
            int m_dstOpNum;
            // immの数
            int m_immNum;

            int m_microOpNum;   // The number of micro-ops for its instruction.
            int m_microOpIndex; // The position of this micro op in its instruction.

            // ニーモニック
            const char* m_mnemonic;

            // m_srcReg, m_imm, m_dstReg と，m_src, m_dstの対応づけ
            // m_srcReg[i]番レジスタ を m_src[ m_srcRegOpMap ] に格納する．他も同様
            boost::array<int, MaxSrcCount> m_srcRegOpMap;
            boost::array<int, MaxDstCount> m_dstRegOpMap;
            boost::array<int, MaxImmCount> m_immOpMap;
        };

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif

