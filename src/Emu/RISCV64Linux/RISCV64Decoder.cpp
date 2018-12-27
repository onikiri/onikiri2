// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Masahiro Goshima.
// Copyright (c) 2005-2018 Ryota Shioya.
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

#include "Emu/RISCV64Linux/RISCV64Decoder.h"
#include "Emu/RISCV64Linux/RISCV64Info.h"

#include "SysDeps/Endian.h"
#include "Emu/Utility/DecoderUtility.h"


using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::RISCV64Linux;

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

    static const int OP_IMMW = 0x1b;    // Integer immediate word
    static const int OP_INTW = 0x3b;    // Integer word

    static const int OP_ATOMIC = 0x2f;  // Atomic memory operations

    static const int OP_FLD = 0x07;     // Float load
    static const int OP_FST = 0x27;     // Float store
    static const int OP_FMADD = 0x43;   // Float mul add
    static const int OP_FMSUB = 0x47;   // Float mul sub
    static const int OP_FNMSUB = 0x4b;  // Float neg mul sub
    static const int OP_FNMADD = 0x4f;  // Float neg mul add
    static const int OP_FLOAT = 0x53;   // Float
}

RISCV64Decoder::DecodedInsn::DecodedInsn()
{
    clear();
}

void RISCV64Decoder::DecodedInsn::clear()
{
    CodeWord = 0;

    std::fill(Imm.begin(), Imm.end(), 0);
    std::fill(Reg.begin(), Reg.end(), -1);
}


RISCV64Decoder::RISCV64Decoder()
{
}

void RISCV64Decoder::Decode(u32 codeWord, DecodedInsn* out)
{
    out->clear();
    out->CodeWord = codeWord;

    u32 opcode = codeWord & 0x7f;


    switch (opcode) {
    case OP_AUIPC:
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
        out->Imm[0] = ExtractBits<u64>(codeWord, 12, 20, true);    // imm
        break;

    case OP_LUI:
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
        out->Imm[0] = ExtractBits<u64>(codeWord, 12, 20, true);    // imm
        break;

    case OP_IMM:
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5);     // rs1
        out->Imm[0] = ExtractBits<u64>(codeWord, 20, 12, true);    // imm
        break;

    case OP_INT:
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5);     // rs1
        out->Reg[2] = ExtractBits(codeWord, 20, 5);     // rs2
        break;

    case OP_JAL:
    {
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd

        u64 imm =
            (ExtractBits<u64>(codeWord, 21, 10) << 0) |
            (ExtractBits<u64>(codeWord, 20, 1) << 10) |
            (ExtractBits<u64>(codeWord, 12, 8) << 11) |
            (ExtractBits<u64>(codeWord, 31, 1) << 19);

        out->Imm[0] = ExtractBits<u64>(imm, 0, 20, true) << 1;
        break;
    }

    case OP_JALR:
    {
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5);     // rs1
        out->Imm[0] = ExtractBits<u64>(codeWord, 20, 12, true);
        break;
    }

    case OP_BR:
    {
        out->Reg[0] = ExtractBits(codeWord, 15, 5);      // rs1
        out->Reg[1] = ExtractBits(codeWord, 20, 5);      // rs2

        u64 imm =
            (ExtractBits<u64>(codeWord, 8,  4) << 0) |
            (ExtractBits<u64>(codeWord, 25, 6) << 4) |
            (ExtractBits<u64>(codeWord, 7,  1) << 10) |
            (ExtractBits<u64>(codeWord, 31, 1) << 11);

        out->Imm[0] = ExtractBits<u64>(imm, 0, 12, true) << 1;
        break;
    }

    case OP_LD:
    {
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5);     // rs1
        out->Imm[0] = ExtractBits<u64>(codeWord, 20, 12, true);
        break;
    }

    case OP_ST:
    {
        out->Reg[0] = ExtractBits(codeWord, 15, 5);     // rs1
        out->Reg[1] = ExtractBits(codeWord, 20, 5);     // rs2

        u64 imm =
            (ExtractBits<u64>(codeWord, 7,  5) << 0) |
            (ExtractBits<u64>(codeWord, 25, 7) << 5);

        out->Imm[0] = ExtractBits<u64>(imm, 0, 12, true);
        break;
    }

    case OP_ECALL:
        if (ExtractBits<u64>(codeWord, 12, 3) == 0x1 || 
            ExtractBits<u64>(codeWord, 12, 3) == 0x2 || 
            ExtractBits<u64>(codeWord, 12, 3) == 0x3
        ) {
            out->Reg[0] = ExtractBits(codeWord, 7, 5);          // rd 
            out->Reg[1] = ExtractBits(codeWord, 15, 5);         // rs1
            out->Imm[0] = ExtractBits(codeWord, 20, 12);   // csr
        }
        else if (
            ExtractBits<u64>(codeWord, 12, 3) == 0x5 || 
            ExtractBits<u64>(codeWord, 12, 3) == 0x6 || 
            ExtractBits<u64>(codeWord, 12, 3) == 0x7
        ) {
            out->Reg[0] = ExtractBits(codeWord, 7, 5);              // rd
            out->Imm[0] = ExtractBits<u64>(codeWord, 15, 5);  // imm
            out->Imm[1] = ExtractBits(codeWord, 20, 12);       // csr
        }
        break;

    case OP_IMMW:
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5);     // rs1
        out->Imm[0] = ExtractBits<u64>(codeWord, 20, 12, true);    // imm
        break;

    case OP_INTW:
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5);     // rs1
        out->Reg[2] = ExtractBits(codeWord, 20, 5);     // rs2
        break;

    case OP_ATOMIC:
        out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5);     // rs1
        out->Reg[2] = ExtractBits(codeWord, 20, 5);     // rs2
        break;

    case OP_FLD:
        out->Reg[0] = ExtractBits(codeWord, 7, 5) + 32;      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5);     // rs1
        out->Imm[0] = ExtractBits<u64>(codeWord, 20, 12, true);
        break;

    case OP_FST:
    {
        out->Reg[0] = ExtractBits(codeWord, 15, 5);     // rs1
        out->Reg[1] = ExtractBits(codeWord, 20, 5) + 32;     // rs2

        u64 imm =
            (ExtractBits<u64>(codeWord, 7, 5) << 0) |
            (ExtractBits<u64>(codeWord, 25, 7) << 5);
        out->Imm[0] = ExtractBits<u64>(imm, 0, 12, true);
        break;
    }

    case OP_FMADD:
        out->Reg[0] = ExtractBits(codeWord, 7, 5) + 32;      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5) + 32;     // rs1
        out->Reg[2] = ExtractBits(codeWord, 20, 5) + 32;     // rs2
        out->Reg[3] = ExtractBits(codeWord, 27, 5) + 32;     // rs3
        break;

    case OP_FMSUB:
        out->Reg[0] = ExtractBits(codeWord, 7, 5) + 32;      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5) + 32;     // rs1
        out->Reg[2] = ExtractBits(codeWord, 20, 5) + 32;     // rs2
        out->Reg[3] = ExtractBits(codeWord, 27, 5) + 32;     // rs3
        break;

    case OP_FNMSUB:
        out->Reg[0] = ExtractBits(codeWord, 7, 5) + 32;      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5) + 32;     // rs1
        out->Reg[2] = ExtractBits(codeWord, 20, 5) + 32;     // rs2
        out->Reg[3] = ExtractBits(codeWord, 27, 5) + 32;     // rs3
        break;

    case OP_FNMADD:
        out->Reg[0] = ExtractBits(codeWord, 7, 5) + 32;      // rd
        out->Reg[1] = ExtractBits(codeWord, 15, 5) + 32;     // rs1
        out->Reg[2] = ExtractBits(codeWord, 20, 5) + 32;     // rs2
        out->Reg[3] = ExtractBits(codeWord, 27, 5) + 32;     // rs3
        break;

    case OP_FLOAT: //正しい？
        if(ExtractBits<u64>(codeWord, 26, 6) == 0x38){
            out->Reg[0] = ExtractBits(codeWord, 7, 5);           // rd 整数レジスタ
            out->Reg[1] = ExtractBits(codeWord, 15, 5) + 32;     // rs1
        }
        else if(ExtractBits<u64>(codeWord, 26, 6) == 0x3c){
          out->Reg[0] = ExtractBits(codeWord, 7, 5) + 32;      // rd 
          out->Reg[1] = ExtractBits(codeWord, 15, 5);          // rs1 整数レジスタ
        }
        else{
          out->Reg[0] = ExtractBits(codeWord, 7, 5) + 32;      // rd
            out->Reg[1] = ExtractBits(codeWord, 15, 5) + 32;     // rs1
            out->Reg[2] = ExtractBits(codeWord, 20, 5) + 32;     // rs2
        }
        break;

    default:
        //THROW_RUNTIME_ERROR("Unknown op code");
        break;
    }
    for (int reg : out->Reg) {
        ASSERT( -1 <= reg && reg < RISCV64Info::RegisterCount, "The decoded register number (%d) is out of range.", reg );
    }
}
