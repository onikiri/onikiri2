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

#include "Emu/RISCV64Linux/RISCV64Decoder.h"

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

  	static const int OP_FLD = 0x07;     // Float load
  	static const int OP_FST = 0x27;     // Float store
  	static const int OP_FMADD = 0x43;   // Float mul add
  	static const int OP_FMSUB = 0x47;   // Float mul sub
  	static const int OP_FNMSUB = 0x4b;  // Float neg mul sub
  	static const int OP_FNMADD = 0x4f;  // Float neg mul add
  	static const int OP_FLOAT = 0x53;   // Float

    static constexpr int OP_C_ADDI4SPN  = 0x0000;
    static constexpr int OP_C_FLD       = 0x2000;
    static constexpr int OP_C_LW        = 0x4000;
    static constexpr int OP_C_LD        = 0x6000;
    static constexpr int OP_C_FSD       = 0xa000;
    static constexpr int OP_C_SW        = 0xc000;
    static constexpr int OP_C_SD        = 0xe000;
    static constexpr int OP_C_ADDI      = 0x0001;
    static constexpr int OP_C_ADDIW     = 0x2001;
    static constexpr int OP_C_LI        = 0x4001;
    static constexpr int OP_C_LUIorSP   = 0x6001;
    static constexpr int OP_C_MISC_ALU  = 0x8001;
    static constexpr int OP_C_J         = 0xa001;
    static constexpr int OP_C_BEQZ      = 0xc001;
    static constexpr int OP_C_BENZ      = 0xe001;
    static constexpr int OP_C_SLLI      = 0x0002;
    static constexpr int OP_C_FLDSP     = 0x2002;
    static constexpr int OP_C_LWSP      = 0x4002;
    static constexpr int OP_C_LDSP      = 0x6002;
    static constexpr int OP_C_JALRMVADD = 0x8002;
    static constexpr int OP_C_FSDSP     = 0xa002;
    static constexpr int OP_C_SWSP      = 0xc002;
    static constexpr int OP_C_SDSP      = 0xe002;


    constexpr int PopulareRegisterOffset = 8;

    int ExtractCompressed_rs2(u32 codeWord) { return ExtractBits(codeWord, 2, 3) + PopulareRegisterOffset; }
    int ExtractCompressed_rs1(u32 codeWord) { return ExtractBits(codeWord, 7, 3) + PopulareRegisterOffset; }
    int Extract_rs2(u32 codeWord) { return ExtractBits(codeWord, 2, 5); }
    int Extract_rs1(u32 codeWord) { return ExtractBits(codeWord, 7, 5); }
    u64 ExtractCompressed_imm6(u32 codeWord) { return ExtractBits<u64>(codeWord, 12, 1, true) << 5 | ExtractBits(codeWord, 2, 5); }
    u64 ExtractDispX8_Quadrant0(u32 codeWord) { return ExtractBits<u64>(codeWord, 5, 2, true) << 6 | ExtractBits(codeWord, 10, 3) << 3; }
    u64 ExtractDispX4_Quadrant0(u32 codeWord) { return ExtractBits<u64>(codeWord, 5, 1, true) << 6 | ExtractBits(codeWord, 10, 3) << 3 | ExtractBits(codeWord, 6, 1) << 2; }

    enum C_SP { LOAD, STORE };
    template<C_SP IsStore>
        u64 ExtractDispX8_Quadrant2(u32 codeWord) {
            u64 ix = IsStore ? Extract_rs1(codeWord) : Extract_rs2(codeWord);
            return ExtractBits(ix, 3, 2) << 3
                 | ExtractBits(codeWord, 12, 1) << 5
                 | ExtractBits<u64>(ix, 0, 3, true) << 6;
        }
    template<C_SP IsStore>
        u64 ExtractDispX4_Quadrant2(u32 codeWord) {
            u64 ix = IsStore ? Extract_rs1(codeWord) : Extract_rs2(codeWord);
            return ExtractBits(ix, 2, 3) << 2
                 | ExtractBits(codeWord, 12, 1) << 5
                 | ExtractBits<u64>(ix, 0, 2, true) << 6;
        }
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

void RISCV64Decoder::DecodeCompressedInstruction(u32 codeWord, DecodedInsn* out)
{
    switch( codeWord & 0xe003 ) {
    case OP_C_ADDI4SPN:
        out->Reg[0] = ExtractCompressed_rs1(codeWord);
        out->Imm[0] = ExtractBits(codeWord, 6, 1) << 2
                    | ExtractBits(codeWord, 5, 1) << 3
                    | ExtractBits(codeWord, 11, 2) << 4
                    | ExtractBits<u64>(codeWord, 7, 4, true) << 6;
        break;
    case OP_C_FLD:
        out->Reg[0] = ExtractCompressed_rs2(codeWord) + 32;
        out->Reg[1] = ExtractCompressed_rs1(codeWord);
        out->Imm[0] = ExtractDispX8_Quadrant0(codeWord);
        break;
    case OP_C_LW:
        out->Reg[0] = ExtractCompressed_rs2(codeWord);
        out->Reg[1] = ExtractCompressed_rs1(codeWord);
        out->Imm[0] = ExtractDispX4_Quadrant0(codeWord);
        break;
    case OP_C_LD:
        out->Reg[0] = ExtractCompressed_rs2(codeWord);
        out->Reg[1] = ExtractCompressed_rs1(codeWord);
        out->Imm[0] = ExtractDispX8_Quadrant0(codeWord);
        break;
    case OP_C_FSD:
        out->Reg[0] = ExtractCompressed_rs1(codeWord);
        out->Reg[1] = ExtractCompressed_rs2(codeWord) + 32;
        out->Imm[0] = ExtractDispX8_Quadrant0(codeWord);
        break;
    case OP_C_SW:
        out->Reg[0] = ExtractCompressed_rs1(codeWord);
        out->Reg[1] = ExtractCompressed_rs2(codeWord);
        out->Imm[0] = ExtractDispX4_Quadrant0(codeWord);
        break;
    case OP_C_SD:
        out->Reg[0] = ExtractCompressed_rs1(codeWord);
        out->Reg[1] = ExtractCompressed_rs2(codeWord);
        out->Imm[0] = ExtractDispX8_Quadrant0(codeWord);
        break;
    case OP_C_ADDI:
    case OP_C_ADDIW:
    case OP_C_LI:
        out->Reg[0] = Extract_rs1(codeWord);
        out->Imm[0] = ExtractCompressed_imm6(codeWord);
        break;
    case OP_C_LUIorSP:
        out->Reg[0] = Extract_rs1(codeWord);
        if( out->Reg[0] == 2 ) {
            // ADDI16SP
            out->Imm[0] = ExtractBits(codeWord, 6, 1) << 4
                        | ExtractBits(codeWord, 2, 1) << 5
                        | ExtractBits(codeWord, 4, 1) << 6
                        | ExtractBits(codeWord, 3, 2) << 7
                        | ExtractBits<u64>(codeWord, 12, 1) << 9;
        } else {
            out->Imm[0] = ExtractCompressed_imm6(codeWord);
        }
        break;
    case OP_C_MISC_ALU:
        if( ExtractBits(codeWord, 10, 2) != 3 ) {
            // C.SRLI, C.SRAI, C.ANDI
            out->Reg[0] = ExtractCompressed_rs1(codeWord);
            out->Imm[0] = ExtractCompressed_imm6(codeWord);
        } else {
            // C.SUB, C.XOR, C.OR, C.AND, C.SUBW, C.ADDW
            out->Reg[0] = ExtractCompressed_rs1(codeWord);
            out->Reg[1] = ExtractCompressed_rs2(codeWord);
        }
        break;
    case OP_C_J:
        out->Imm[0] = ExtractBits(codeWord, 3, 3) << 1
                    | ExtractBits(codeWord, 11, 1) << 4
                    | ExtractBits(codeWord, 2, 1) << 5
                    | ExtractBits(codeWord, 7, 1) << 6
                    | ExtractBits(codeWord, 6, 1) << 7
                    | ExtractBits(codeWord, 9, 2) << 8
                    | ExtractBits(codeWord, 8, 1) << 10
                    | ExtractBits<u64>(codeWord, 12, 1, true) << 11;
        break;
    case OP_C_BEQZ:
    case OP_C_BENZ:
        out->Reg[0] = ExtractCompressed_rs1(codeWord);
        out->Imm[0] = ExtractBits(codeWord, 3, 2) << 1
                    | ExtractBits(codeWord, 10, 2) << 3
                    | ExtractBits(codeWord, 2, 1) << 5
                    | ExtractBits(codeWord, 5, 2) << 6
                    | ExtractBits<u64>(codeWord, 12, 1) << 8;
        break;
    case OP_C_SLLI:
        out->Reg[0] = Extract_rs1(codeWord);
        out->Imm[0] = ExtractCompressed_imm6(codeWord);
        break;
    case OP_C_FLDSP:
        out->Reg[0] = Extract_rs1(codeWord) + 32;
        out->Imm[0] = ExtractDispX8_Quadrant2<C_SP::LOAD>(codeWord);
        break;
    case OP_C_LWSP:
        out->Reg[0] = Extract_rs1(codeWord);
        out->Imm[0] = ExtractDispX4_Quadrant2<C_SP::LOAD>(codeWord);
        break;
    case OP_C_LDSP:
        out->Reg[0] = Extract_rs1(codeWord);
        out->Imm[0] = ExtractDispX8_Quadrant2<C_SP::LOAD>(codeWord);
        break;
    case OP_C_JALRMVADD:
        out->Reg[0] = Extract_rs1(codeWord);
        out->Reg[1] = Extract_rs2(codeWord);
        break;
    case OP_C_FSDSP:
        out->Reg[0] = Extract_rs2(codeWord) + 32;
        out->Imm[0] = ExtractDispX8_Quadrant2<C_SP::STORE>(codeWord);
        break;
    case OP_C_SWSP:
        out->Reg[0] = Extract_rs2(codeWord);
        out->Imm[0] = ExtractDispX4_Quadrant2<C_SP::STORE>(codeWord);
        break;
    case OP_C_SDSP:
        out->Reg[0] = Extract_rs2(codeWord);
        out->Imm[0] = ExtractDispX8_Quadrant2<C_SP::STORE>(codeWord);
        break;
    }
}


void RISCV64Decoder::Decode(u32 codeWord, DecodedInsn* out)
{
    out->clear();
    out->CodeWord = codeWord;

    u32 opcode = codeWord & 0x7f;

    if ((opcode&3) != 3) { DecodeCompressedInstruction(codeWord, out); }

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
		if (ExtractBits<u64>(codeWord, 12, 3) == 0x1 || ExtractBits<u64>(codeWord, 12, 3) == 0x2 || ExtractBits<u64>(codeWord, 12, 3) == 0x3) {
			out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd 
			out->Reg[1] = ExtractBits(codeWord, 15, 5);     // rs1
			out->Reg[2] = ExtractBits(codeWord, 25, 12) + 65;     // csr
		}
		else if (ExtractBits<u64>(codeWord, 12, 3) == 0x5 || ExtractBits<u64>(codeWord, 12, 3) == 0x6 || ExtractBits<u64>(codeWord, 12, 3) == 0x7) {
			out->Reg[0] = ExtractBits(codeWord, 7, 5);      // rd
			out->Imm[0] = ExtractBits<u64>(codeWord, 15, 5, true);      // imm
			out->Reg[1] = ExtractBits(codeWord, 20, 12) + 65;     // csr
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

	case OP_FLD:
		out->Reg[0] = ExtractBits(codeWord, 7, 5) + 32;      // rd
		out->Reg[1] = ExtractBits(codeWord, 15, 5);     // rs1
		out->Imm[0] = ExtractBits<u64>(codeWord, 20, 12, true);
		break;

	case OP_FST:
	{
		out->Reg[0] = ExtractBits(codeWord, 15, 5);     // rs1
		out->Reg[1] = ExtractBits(codeWord, 20, 5);     // rs2

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
        break;
    }
}
