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

#include "Emu/AlphaLinux/Alpha64Decoder.h"

#include "SysDeps/Endian.h"
#include "Emu/Utility/DecoderUtility.h"


using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::AlphaLinux;

namespace {
    // 命令の種類
    enum InsnType {
        UNDEF,              // 未定義，または予約
        MEMORY_ADDR,        // メモリ形式 (アドレス計算)
        MEMORY_LOAD,        // メモリ形式 (ロード)
        MEMORY_STORE,       // メモリ形式 (ストア)
        MEMORY_LOAD_FLOAT,  // 浮動小数点メモリ形式 (ロード)
        MEMORY_STORE_FLOAT, // 浮動小数点メモリ形式 (ストア)
        MEMORY_FUNC,        // 変位を機能コードとして用いるメモリ形式 (MB等)
        MEMORY_JMP,         // ジャンプ命令 (JSR等)
        BRANCH,             // 分岐形式
        BRANCH_FLOAT,       // 分岐形式 (浮動小数点)
        BRANCH_SAVE,        // PCを保存する分岐形式 (BR, BSR)
        OPERATION_INT,      // 操作形式 (整数)
        OPERATION_FLOAT,    // 操作形式 (浮動小数)
        PAL                 // PALコード形式
    };

    const InsnType UND = UNDEF;
    const InsnType LDA = MEMORY_ADDR;
    const InsnType LD = MEMORY_LOAD;
    const InsnType ST = MEMORY_STORE;
    const InsnType LDF = MEMORY_LOAD_FLOAT;
    const InsnType STF = MEMORY_STORE_FLOAT;
    const InsnType MEMF = MEMORY_FUNC;
    const InsnType MEMJ = MEMORY_JMP;
    const InsnType BR = BRANCH;
    const InsnType BRF = BRANCH_FLOAT;
    const InsnType BRS = BRANCH_SAVE;
    const InsnType OPI = OPERATION_INT;
    const InsnType OPF = OPERATION_FLOAT;

    // 命令コードと種類の対応
    InsnType OpCodeToInsnType[64] =
    {
        // 0x00
        PAL , UND , UND , UND , UND , UND , UND , UND ,
        // 0x08
        LDA , LDA , LD  , LD  , LD  , ST ,  ST ,  ST  , 
        // 0x10
        OPI , OPI , OPI , OPI , OPF , OPF , OPF , OPF , 
        // 0x18
        MEMF, UND , MEMJ, UND , OPI , UND , UND , UND , 
        // 0x20
        LDF , LDF , LDF , LDF , STF , STF , STF , STF , 
        // 0x28
        LD  , LD  , LD  , LD  , ST  , ST  , ST  , ST  , 
        // 0x30
        BRS , BRF , BRF , BRF , BRS , BRF , BRF , BRF , 
        // 0x38
        BR  , BR  , BR  , BR  , BR  , BR  , BR  , BR  , 
    };
}

Alpha64Decoder::DecodedInsn::DecodedInsn()
{
    clear();
}

void Alpha64Decoder::DecodedInsn::clear()
{
    CodeWord = 0;

    std::fill(Imm.begin(), Imm.end(), 0);
    std::fill(Reg.begin(), Reg.end(), -1);
}


Alpha64Decoder::Alpha64Decoder()
{
}

void Alpha64Decoder::Decode(u32 codeWord, DecodedInsn* out)
{
    out->clear();

    u32 opcode = (codeWord >> 26) & 0x3f;
    out->CodeWord = codeWord;

    InsnType type = OpCodeToInsnType[opcode];

    switch (type) {
    case PAL:
        out->Imm[1] = ExtractBits(codeWord, 0, 26);
        break;
    case UNDEF:
        break;
    case MEMORY_ADDR:
    case MEMORY_LOAD:
        out->Imm[0] = ExtractBits<u64>(codeWord, 0, 16, true);
        out->Reg[0] = ExtractBits(codeWord, 21, 5);
        out->Reg[1] = ExtractBits(codeWord, 16, 5);
        break;
    case MEMORY_STORE:
        out->Imm[0] = ExtractBits<u64>(codeWord, 0, 16, true);
        out->Reg[2] = ExtractBits(codeWord, 21, 5);
        out->Reg[1] = ExtractBits(codeWord, 16, 5);
        break;
    case MEMORY_LOAD_FLOAT:
        out->Imm[0] = ExtractBits<u64>(codeWord, 0, 16, true);
        out->Reg[0] = ExtractBits(codeWord, 21, 5)+32;  // fp register
        out->Reg[1] = ExtractBits(codeWord, 16, 5);
        break;
    case MEMORY_STORE_FLOAT:
        out->Imm[0] = ExtractBits<u64>(codeWord, 0, 16, true);
        out->Reg[2] = ExtractBits(codeWord, 21, 5)+32;  // fp register
        out->Reg[1] = ExtractBits(codeWord, 16, 5);
        break;
    case MEMORY_FUNC:
        out->Imm[1] = ExtractBits(codeWord, 0, 16);
        out->Reg[0] = ExtractBits(codeWord, 21, 5);
        out->Reg[1] = ExtractBits(codeWord, 16, 5);
        break;
    case MEMORY_JMP:
        out->Imm[0] = ExtractBits(codeWord, 0, 14);
        //out->Imm[0] = ExtractBits(codeWord, 14, 2);
        out->Reg[1] = ExtractBits(codeWord, 16, 5); // Rb
        out->Reg[0] = ExtractBits(codeWord, 21, 5); // Ra
        break;
    case BRANCH:
        out->Imm[0] = ExtractBits<u64>(codeWord, 0, 21, true);
        out->Reg[1] = ExtractBits(codeWord, 21, 5);
        break;
    case BRANCH_FLOAT:
        out->Imm[0] = ExtractBits<u64>(codeWord, 0, 21, true);
        out->Reg[1] = ExtractBits(codeWord, 21, 5)+32;  // fp register
        break;
    case BRANCH_SAVE:
        out->Imm[0] = ExtractBits<u64>(codeWord, 0, 21, true);
        out->Reg[0] = ExtractBits(codeWord, 21, 5);
        break;
    case OPERATION_INT:
        out->Reg[0] = ExtractBits(codeWord, 0, 5);
        out->Imm[1] = ExtractBits(codeWord, 5, 7);
        if (ExtractBits(codeWord, 12, 1))
            out->Imm[0] = ExtractBits(codeWord, 13, 8);
        else
            out->Reg[2] = ExtractBits(codeWord, 16, 5);
        if (ExtractBits(codeWord, 5, 3) == 0 && 
            ExtractBits(codeWord, 9, 3) == 7)   // FTOIx
            out->Reg[1] = ExtractBits(codeWord, 21, 5) + 32;    // fp register
        else
            out->Reg[1] = ExtractBits(codeWord, 21, 5);
        break;
    case OPERATION_FLOAT:
        out->Reg[0] = ExtractBits(codeWord, 0, 5) + 32;
        out->Imm[1] = ExtractBits(codeWord, 5, 11);
        out->Reg[2] = ExtractBits(codeWord, 16, 5) + 32;    // fp register
        if(ExtractBits(codeWord, 5, 4) == 4)    // ITOFx
            out->Reg[1] = ExtractBits(codeWord, 21, 5); // int register
        else
            out->Reg[1] = ExtractBits(codeWord, 21, 5) + 32;
        break;
    default:
        ASSERT(0);  // never reached
    }
}

