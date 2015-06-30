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


#ifndef __OPCLASSCODE_H__
#define __OPCLASSCODE_H__

#include "Types.h"

namespace Onikiri {

    // 命令の種類
    namespace OpClassCode
    {
        enum OpClassCode
        {
            CALL = 0,           //  subroutine call
            CALL_JUMP,          //  indirect subroutine call
            RET,                //  return from subroutine
            RETC,               //  return from subroutine conditional

            INT_BEGIN,
            iBC = INT_BEGIN,    //  integer conditional branch
            iBU,                //  integer unconditional branch
            iJUMP,              //  integer unconditional jump

            iLD,                //  integer  load
            iST,                //  integer  store
            iIMM,               //  integer  set immediate
            iMOV,               //  integer  move
            iALU,               //  integer  Arithmetic/Logic
            iSFT,               // (integer) shift
            iMUL,               //  integer  multiply
            iDIV,               //  integer  divide
            iBYTE,              // (integer) byte ops
            INT_END = iBYTE,

            FLOAT_BEGIN,
            fBC = FLOAT_BEGIN,  //  FP branch (conditional)
            fLD,                //  FP load
            fST,                //  FP store
            fMOV,               //  FP move
            fNEG,               //  FP negate
            fADD,               //  FP add/sub
            fMUL,               //  FP multiply
            fDIV,               //  FP divide
            fCONV,              //  FP convert
            fELEM,              //  FP elementary functions (SQRT, SIN, COS, etc)
            FLOAT_END = fELEM,

            ifCONV,             // INT <-> FP conversion

            syscall,            //  system call
            syscall_branch,     //  system call with branch

            ADDR,               // address calculation

            // EXEC_END = ADDR, // 実行されるinsncodeの最後

            iNOP,               // NOP
            fNOP,               // floating NOP

            UNDEF,              // undefined operation

            other,              //  ???
            
            Code_MAX = other
        };

        // OpClassCode c を，文字列に変換する
        const char* ToString(int c);

        // 文字列 s を，OpClassCode に変換する．対応するOpClassCode が存在しなければ例外を投げる
        int FromString(const char *s);


        // 分岐（jumpを含む）かどうか
        bool IsBranch( int code );

        // Note: 分岐に関して，Conditional, IndirectJump, Call は排他ではない
        //       ReturnとCall/Jumpは排他だが，Conditional Returnは存在する
        // 条件分岐かどうか
        bool IsConditionalBranch( int code );
        bool IsUnconditionalBranch( int code );

        // jump かどうか
        bool IsIndirectJump( int code );

        // call
        bool IsCall( int code );

        // return
        bool IsReturn( int code );

        // サブルーチン
        bool IsSubroutine( int code );

        // メモリ
        bool IsMem( int code );

        // ロード
        bool IsLoad( int code );
        
        // ストア
        bool IsStore( int code );

        // Address calculation
        bool IsAddr( int code );

        // 整数
        bool IsInt( int code );

        // 浮動小数点数
        bool IsFloat( int code );

        // 整数 <-> 浮動小数点数変換
        bool IsIFConversion( int code );

        // システムコールは単体で実行する。すなわち、
        // システムコールを実行する前に上流の命令をすべてリタイアさせて、
        // システムコールがリタイアするまで下流もフェッチしない
        bool IsSyscall( int code );

        // Nop
        bool IsNop( int code );

    }
}; // namespace Onikiri

#endif

