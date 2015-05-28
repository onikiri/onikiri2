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
// ユーザー定義命令デコーダでデコードした命令のラッパ
//

#ifndef EMU_UTILITY_EXTRA_OP_INFO_WRAPPER_H
#define EMU_UTILITY_EXTRA_OP_INFO_WRAPPER_H

#include "Emu/Utility/CommonOpInfo.h"
#include "Emu/Utility/OpEmulationState.h"

namespace Onikiri
{
    namespace EmulatorUtility
    {
        template<class TISAInfo>
        class ExtraOpEmuStateWrapper : public OpStateIF
        {
        private:
            OpEmulationState* m_emuState;

        public:
            ExtraOpEmuStateWrapper( OpEmulationState* emuState ) : m_emuState(emuState)
            {
            }

            ~ExtraOpEmuStateWrapper()
            {
            }

            // OpStateIF
            PC GetPC() const
            {
                return Addr( 
                    m_emuState->GetPID(),
                    m_emuState->GetTID(),
                    m_emuState->GetPC() );
            }

            const u64 GetSrc(const int index) const
            {
                return m_emuState->GetSrc( index );
            }

            const u64 GetDst(const int index) const
            {
                return m_emuState->GetDst( index );
            }

            void SetDst(const int index, const u64 value)
            {
                return m_emuState->SetDst( index, value );
            }

            void SetTakenPC(const PC takenPC)
            {
                m_emuState->SetTakenPC( takenPC.address );
            }

            PC GetTakenPC() const
            {
                return Addr( 
                    m_emuState->GetPID(),
                    m_emuState->GetTID(),
                    m_emuState->GetTakenPC() );
            }

            void SetTaken(const bool taken)
            {
                m_emuState->SetTaken( taken );
            }

            bool GetTaken() const
            {
                return m_emuState->GetTaken();
            }

            // MemIF
            void Read( MemAccess* access )
            {
                m_emuState->GetOpState()->Read( access );
            }

            void Write( MemAccess* access )
            {
                m_emuState->GetOpState()->Write( access );
            }
        };

        template<class TISAInfo>
        class ExtraOpInfoWrapper : public CommonOpInfo<TISAInfo> 
        {
            ExtraOpInfoIF* m_exOp;
        public:
            using CommonOpInfo<TISAInfo>::SetDstRegOpMap;
            using CommonOpInfo<TISAInfo>::SetSrcRegOpMap;
            using CommonOpInfo<TISAInfo>::SetDstRegNum;
            using CommonOpInfo<TISAInfo>::SetSrcRegNum;
            using CommonOpInfo<TISAInfo>::SetDstReg;
            using CommonOpInfo<TISAInfo>::SetSrcReg;
            using CommonOpInfo<TISAInfo>::SetEmulationFunc;
        
            ExtraOpInfoWrapper() : 
                CommonOpInfo<TISAInfo>( OpClass( OpClassCode::UNDEF ) ),
                m_exOp(0)
            {
                SetEmulationFunc( &EmulationFunctionProxy );
            }

            void SetExtraOpInfo( ExtraOpInfoIF* exOp )
            {
                m_exOp = exOp;

                // CommonOpInfo 側に反映
                SetDstRegNum( m_exOp->GetDstNum() );
                SetSrcRegNum( m_exOp->GetSrcNum() );
                
                for( int i = 0; i < m_exOp->GetDstNum(); i++ ){
                    SetDstRegOpMap( i, i );
                    SetDstReg( i, m_exOp->GetDstOperand( i ) );
                }
                for( int i = 0; i < m_exOp->GetSrcNum(); i++ ){
                    SetSrcRegOpMap( i, i );
                    SetSrcReg( i, m_exOp->GetSrcOperand( i ) );
                }
            }

            ExtraOpInfoIF* GetExtraOpInfo() const
            {
                return m_exOp;
            }

            static void EmulationFunctionProxy( OpEmulationState* emuState )
            {
                OpInfo* base = emuState->GetOpInfo();
                ExtraOpInfoWrapper* wrapper = static_cast<ExtraOpInfoWrapper*>(base);
                ASSERT( wrapper != 0 );

                ExtraOpEmuStateWrapper<TISAInfo> tmpState( emuState );

                // 実行
                ExtraOpInfoIF* exOpInfo = wrapper->GetExtraOpInfo();
                exOpInfo->Execute( &tmpState );
            }

            // OpInfo の実装
            const OpClass& GetOpClass() const
            {
                ASSERT( m_exOp != 0 );
                return m_exOp->GetOpClass();
            }
            int GetSrcOperand(const int index) const
            {
                ASSERT( m_exOp != 0 );
                return m_exOp->GetSrcOperand(index);
            }
            int GetDstOperand(const int index) const
            {
                ASSERT( m_exOp != 0 );
                return m_exOp->GetDstOperand(index);
            }
            int GetSrcNum() const
            {
                ASSERT( m_exOp != 0 );
                return m_exOp->GetSrcNum();
            }
            int GetDstNum() const
            {
                ASSERT( m_exOp != 0 );
                return m_exOp->GetDstNum();
            }
            int GetMicroOpNum() const
            {
                ASSERT( m_exOp != 0 );
                return m_exOp->GetMicroOpNum();
            }
            int GetMicroOpIndex() const
            {
                ASSERT( m_exOp != 0 );
                return m_exOp->GetMicroOpIndex();
            }
            const char* GetMnemonic() const
            {
                ASSERT( m_exOp != 0 );
                return m_exOp->GetMnemonic();
            }
        };

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif

