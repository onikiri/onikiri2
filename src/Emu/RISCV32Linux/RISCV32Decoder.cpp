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

    static const int OP_ECALL = 0x73;   // system call

    static const int OP_ATOMIC = 0x2f;  // Atomic memory operations
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

    case OP_LUI:
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
        out->Imm[0] = ExtractBits(codeWord, 12, 20);    // imm
        break;

    case OP_IMM:
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5);     // rs1
        out->Imm[0] = ExtractBits(codeWord, 20, 12, true);    // imm
        break;

    case OP_INT:
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5);     // rs1
        out->Reg[2] = ExtractBits(codeWord, 20, 5);     // rs2
        break;

    case OP_JAL:
    {
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd

        u32 imm =
            (ExtractBits(codeWord, 21, 10) << 0) |
            (ExtractBits(codeWord, 20, 1) << 10) |
            (ExtractBits(codeWord, 12, 8) << 11) |
            (ExtractBits(codeWord, 31, 1) << 19);

        out->Imm[0] = ExtractBits(imm, 0, 20, true) << 1;
        break;
    }

    case OP_JALR:
    {
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5);     // rs1
        out->Imm[0] = ExtractBits(codeWord, 20, 12, true);
        break;
    }

    case OP_BR:
    {
        out->Reg[0] = ExtractBits(codeWord, 15, 5);      // rs1
        out->Reg[1] = ExtractBits(codeWord, 20, 5);      // rs2

        u32 imm =
            (ExtractBits(codeWord, 8,  4) << 0) |
            (ExtractBits(codeWord, 25, 6) << 4) |
            (ExtractBits(codeWord, 7,  1) << 10) |
            (ExtractBits(codeWord, 31, 1) << 11);

        out->Imm[0] = ExtractBits(imm, 0, 12, true) << 1;
        break;
    }

    case OP_LD:
    {
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5);     // rs1
        out->Imm[0] = ExtractBits(codeWord, 20, 12, true);
        break;
    }

    case OP_ST:
    {
        out->Reg[0] = ExtractBits(codeWord, 15, 5);     // rs1
        out->Reg[1] = ExtractBits(codeWord, 20, 5);     // rs2

        u32 imm =
            (ExtractBits(codeWord, 7,  5) << 0) |
            (ExtractBits(codeWord, 25, 7) << 5);
        out->Imm[0] = ExtractBits(imm, 0, 12, true);
        break;
    }

    case OP_ECALL:
    {
        u64 bits12_14 = ExtractBits<u64>(codeWord, 12, 3);
        if (bits12_14 == 0x1 || bits12_14 == 0x2 || bits12_14 == 0x3) {
            // csrrw/csrrs/csrrc
            out->Reg[0] = ExtractBits(codeWord, 7, 5);          // rd
            out->Reg[1] = ExtractBits(codeWord, 15, 5);         // rs1
            out->Imm[0] = ExtractBits(codeWord, 20, 12);        // csr number
        }
        else if (bits12_14 == 0x5 || bits12_14 == 0x6 || bits12_14 == 0x7) {
            // csrrwi/csrrsi/csrrci
            out->Reg[0] = ExtractBits(codeWord, 7, 5);          // rd
            out->Imm[0] = ExtractBits<u64>(codeWord, 15, 5);    // imm
            out->Imm[1] = ExtractBits(codeWord, 20, 12);        // csr number
        }
        else if (bits12_14 == 0x0) {
            // ecall/ebreak
            // These instructions do not have specific operands
        }
        else {
            // Unknown instruction
            // An unknown instruction can be fetchd becahuse instructions are fetched speculatively,
        }
        break;
    }

    case OP_ATOMIC:
        out->Reg[0] = ExtractBits(codeWord, 7, 5);           // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5);          // rs1
        out->Reg[2] = ExtractBits(codeWord, 20, 5);          // rs2
        break;
    default:
        break;
    }
}

