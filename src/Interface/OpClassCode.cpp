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
#include "Interface/OpClassCode.h"

using namespace std;
using namespace Onikiri;

namespace {
    struct CodeStrElem {
        int         code;
        const char* str;
        bool operator<(const CodeStrElem& cs) const {
            return code < cs.code;
        }
    };
#define CS_ELEM(name) {OpClassCode::name, #name}
    static struct CodeStrElem CodeStr[] = {
        CS_ELEM(CALL),
        CS_ELEM(CALL_JUMP),
        CS_ELEM(RET),
        CS_ELEM(RETC),
        CS_ELEM(iBC),
        CS_ELEM(iBU),
        CS_ELEM(iJUMP),
        CS_ELEM(iLD),
        CS_ELEM(iST),
        CS_ELEM(iIMM),
        CS_ELEM(iMOV),
        CS_ELEM(iALU),
        CS_ELEM(iSFT),
        CS_ELEM(iMUL),
        CS_ELEM(iDIV),
        CS_ELEM(iBYTE),
        CS_ELEM(fBC),
        CS_ELEM(fLD),
        CS_ELEM(fST),
        CS_ELEM(fMOV),
        CS_ELEM(fNEG),
        CS_ELEM(fADD),
        CS_ELEM(fMUL),
        CS_ELEM(fDIV),
        CS_ELEM(fCONV),
        CS_ELEM(fELEM),
        CS_ELEM(ifCONV),
        CS_ELEM(syscall),
        CS_ELEM(syscall_branch),
        CS_ELEM(ADDR),
        CS_ELEM(iNOP),
        CS_ELEM(fNOP),
        CS_ELEM(UNDEF),
        CS_ELEM(other),
    };
#undef CS_ELEM
    const CodeStrElem* CodeStrBegin = &CodeStr[0];
    const CodeStrElem* CodeStrEnd = CodeStrBegin + sizeof(CodeStr)/sizeof(CodeStr[0]);
}

namespace Onikiri
{
    namespace OpClassCode
    {

        const char* ToString(int c)
        {
            CodeStrElem cs;
            cs.code = c;
            const CodeStrElem* e = lower_bound(CodeStrBegin, CodeStrEnd, cs);
            
            if (e == CodeStrEnd || e->code != c) {
                return "(unknown OpClassCode)";
            }
            else {
                return e->str;
            }
        }

        int FromString(const char *s)
        {
            for (const CodeStrElem* e = CodeStrBegin; e != CodeStrEnd; ++e) {
                if (strcmp(e->str, s) == 0)
                    return e->code;
            }

            THROW_RUNTIME_ERROR("unknown code (%s).", s);
            return -1;
        }

        //
        // ---
        //

        // 分岐（jumpを含む）かどうか
        bool IsBranch(int code)         
        {
            return IsConditionalBranch(code) || IsUnconditionalBranch(code);
        }

        // Note: 分岐に関して，Conditional, IndirectJump, Call は排他ではない
        //       ReturnとCall/Jumpは排他だが，Conditional Returnは存在する
        // 条件分岐かどうか
        bool IsConditionalBranch(int code)
        {
            return
                code == iBC ||
                code == fBC || 
                code == RETC;
        }

        bool IsUnconditionalBranch(int code)
        {
            return
                IsCall(code) || 
                code == RET ||
                code == iBU || 
                code == iJUMP || 
                code == syscall_branch;
        }

        // jump かどうか
        bool IsIndirectJump(int code)
        {
            return
                code == CALL_JUMP || 
                code == iJUMP;
        }

        // call
        bool IsCall(int code)
        {
            return 
                code == CALL || 
                code == CALL_JUMP;
        }

        // return
        bool IsReturn(int code)
        {
            return 
                code == RET || 
                code == RETC;
        }

        bool IsSubroutine(int code)
        {
            return 
                IsCall(code) || 
                code == RET ||
                code == RETC;       
        }

        // メモリ
        bool IsMem(int code)
        {
            return IsLoad(code) || IsStore(code);
        }

        // ロード
        bool IsLoad(int code)
        {
            return code == iLD || code == fLD;
        }

        // ストア
        bool IsStore(int code)
        {
            return code == iST || code == fST;
        }

        // Address calculation
        bool IsAddr( int code )
        {
            return code == ADDR;
        }

        // 整数
        bool IsInt(int code)
        {
            return 
                (code >= INT_BEGIN) &&
                (code <= INT_END);
        }

        // 浮動小数点数
        bool IsFloat(int code)
        {
            return 
                (code >= FLOAT_BEGIN) && 
                (code <= FLOAT_END);
        }

        // 整数 <-> 浮動小数点数変換
        bool IsIFConversion( int code )
        {
            return code == ifCONV;
        }

        // システムコールは単体で実行する。すなわち、
        // システムコールを実行する前に上流の命令をすべてリタイアさせて、
        // システムコールがリタイアするまで下流もフェッチしない
        bool IsSyscall(int code)
        {
            return 
                code == syscall ||
                code == syscall_branch || 
                code == UNDEF;  
        }

        bool IsNop(int code)
        {
            return
                code == iNOP ||
                code == fNOP;
        }

    }   // namespace OpClassCode

}   // namespace Onikiri
    
