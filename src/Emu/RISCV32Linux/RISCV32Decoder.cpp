// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Masahiro Goshima.
// Copyright (c) 2005-2017 Ryota Shioya.
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

#include "Emu/RISCV32Linux/RISCV32Decoder.h"

#include "SysDeps/Endian.h"
#include "Emu/Utility/DecoderUtility.h"


using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::RISCV32Linux;

namespace {

    // Op code
    static const int OP_IMM = 0x13;     // Integer immediate
    static const int OP_INT = 0x33;     // Integer
    static const int OP_LUI = 0x37;     // Load upper immediate (LUI)
    static const int OP_AUIPC = 0x17;   // Add upper immediate to pc (AUIPC)

    static const int OP_JAL = 0x6f;     // Jump and link (JAL)
    static const int OP_JALR = 0x67;    // Jump and link register (JALR)

    static const int OP_BR = 0x63;      // Branch
    static const int OP_LD = 0x03;      // Load
    static const int OP_ST = 0x23;      // Store
}

RISCV32Decoder::DecodedInsn::DecodedInsn()
{
    clear();
}

void RISCV32Decoder::DecodedInsn::clear()
{
    CodeWord = 0;

    std::fill(Imm.begin(), Imm.end(), 0);
    std::fill(Reg.begin(), Reg.end(), -1);
}


RISCV32Decoder::RISCV32Decoder()
{
}

void RISCV32Decoder::Decode(u32 codeWord, DecodedInsn* out)
{
    out->clear();
    out->CodeWord = codeWord;

    u32 opcode = codeWord & 0x7f;


    switch (opcode) {
    case OP_AUIPC:
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
        out->Imm[0] = ExtractBits(codeWord, 12, 20);    // imm
        break;

    case OP_IMM:
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5);     // rs1
        out->Imm[0] = ExtractBits(codeWord, 20, 12, true);    // imm
        break;
    /*

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
    */
    default:
        break;
    }
}

