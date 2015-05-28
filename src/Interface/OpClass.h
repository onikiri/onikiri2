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
// 命令の種類をあらわすクラス
// OpClassCode の値からOpClass の挙動は一意に定まるため，
// インターフェースの一部として実装
//

#ifndef __OPCLASS_H__
#define __OPCLASS_H__

#include <string>
#include "Types.h"
#include "SysDeps/Inline.h"
#include "Interface/OpClassCode.h"

namespace Onikiri 
{

    class OpClass
    {
        OpClassCode::OpClassCode m_code;
        int m_opClassBits;

        static const int MASK_BRANCH                = 1 << 0;
        static const int MASK_CONDITIONAL_BRANCH    = 1 << 1;
        static const int MASK_UNCONDITIONAL_BRANCH  = 1 << 2;
        static const int MASK_INDIRECT_JUMP         = 1 << 3;
        static const int MASK_CALL                  = 1 << 4;
        static const int MASK_RETURN                = 1 << 5;
        static const int MASK_SUBROUTINE            = 1 << 6;
        static const int MASK_MEM       = 1 << 7;
        static const int MASK_LOAD      = 1 << 8;
        static const int MASK_STORE     = 1 << 9;
        static const int MASK_ADDR      = 1 << 10;
        static const int MASK_INT       = 1 << 11;
        static const int MASK_FLOAT     = 1 << 12;
        static const int MASK_IFCONV    = 1 << 13;
        static const int MASK_SYSCALL   = 1 << 14;
        static const int MASK_NOP       = 1 << 15;

        INLINE bool BitTest( int mask ) const
        {
            return m_opClassBits & mask ? true : false;
        }

    public:
        OpClass( OpClassCode::OpClassCode code ) : m_code(code)
        {
            m_opClassBits = 0;
            m_opClassBits |= OpClassCode::IsBranch( code )              ? MASK_BRANCH : 0;
            m_opClassBits |= OpClassCode::IsConditionalBranch( code )   ? MASK_CONDITIONAL_BRANCH : 0;
            m_opClassBits |= OpClassCode::IsUnconditionalBranch( code ) ? MASK_UNCONDITIONAL_BRANCH : 0;
            m_opClassBits |= OpClassCode::IsIndirectJump( code )        ? MASK_INDIRECT_JUMP : 0;
            m_opClassBits |= OpClassCode::IsCall( code )                ? MASK_CALL      : 0;
            m_opClassBits |= OpClassCode::IsReturn( code )              ? MASK_RETURN    : 0;
            m_opClassBits |= OpClassCode::IsSubroutine( code )          ? MASK_SUBROUTINE : 0;
            m_opClassBits |= OpClassCode::IsMem( code )                 ? MASK_MEM      : 0;
            m_opClassBits |= OpClassCode::IsLoad( code )                ? MASK_LOAD     : 0;
            m_opClassBits |= OpClassCode::IsStore( code )               ? MASK_STORE    : 0;
            m_opClassBits |= OpClassCode::IsAddr( code )                ? MASK_ADDR     : 0;
            m_opClassBits |= OpClassCode::IsInt( code )                 ? MASK_INT      : 0;
            m_opClassBits |= OpClassCode::IsFloat( code )               ? MASK_FLOAT    : 0;
            m_opClassBits |= OpClassCode::IsIFConversion( code )        ? MASK_IFCONV   : 0;
            m_opClassBits |= OpClassCode::IsSyscall( code )             ? MASK_SYSCALL  : 0;
            m_opClassBits |= OpClassCode::IsNop( code )                 ? MASK_NOP      : 0;
        }

        ~OpClass() {}

        // 命令の種類を連番で返す
        OpClassCode::OpClassCode GetCode() const
        {
            return m_code;
        }

        // 分岐（jumpを含む）かどうか
        bool IsBranch() const
        {   return BitTest( MASK_BRANCH );  }

        // Note: 分岐に関して，Conditional, IndirectJump, Call は排他ではない
        //       ReturnとCall/Jumpは排他だが，Conditional Returnは存在する
        // 条件分岐かどうか
        bool IsConditionalBranch()  const   
        {   return BitTest( MASK_CONDITIONAL_BRANCH );      }
        
        bool IsUnconditionalBranch() const  
        {   return BitTest( MASK_UNCONDITIONAL_BRANCH );    }

        // jump かどうか
        bool IsIndirectJump()   const       
        {   return BitTest( MASK_INDIRECT_JUMP );           }

        // call
        bool IsCall() const 
        {   return BitTest( MASK_CALL );        }

        // return
        bool IsReturn()     const 
        {   return BitTest( MASK_RETURN );      }

        bool IsSubroutine() const
        {   return BitTest( MASK_SUBROUTINE );  }

        // メモリ
        bool IsMem() const  
        {   return BitTest( MASK_MEM );     }

        // ロード
        bool IsLoad() const
        {   return BitTest( MASK_LOAD );    }

        // ストア
        bool IsStore() const
        {   return BitTest( MASK_STORE );   }

        // ロード
        bool IsAddr() const
        {   return BitTest( MASK_ADDR );    }

        // 整数
        bool IsInt() const 
        {   return BitTest( MASK_INT );     }

        // 浮動小数点数
        bool IsFloat() const 
        {   return BitTest( MASK_FLOAT );   }

        // 整数 <-> 浮動小数点数変換
        bool IsIFConversion() const 
        {   return BitTest( MASK_IFCONV );  }

        // システムコールは単体で実行する。すなわち、
        // システムコールを実行する前に上流の命令をすべてリタイアさせて、
        // システムコールがリタイアするまで下流もフェッチしない
        bool IsSyscall() const
        {   return BitTest( MASK_SYSCALL ); }

        // NOP
        bool IsNop() const  
        {   return BitTest( MASK_NOP );     }


        static int GetMaxCode() 
        {
            return OpClassCode::Code_MAX;
        }

        // デバッグのために文字列にする
        const std::string ToString() const
        {
            return OpClassCode::ToString( m_code );
        }
    };

}; // namespace Onikiri

#endif // __OPCLASS_H__

