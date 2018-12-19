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

#include "Emu/RISCV64Linux/RISCV64Converter.h"

//#include "Emu/Utility/GenericOperation.h"
#include "Emu/Utility/OpEmulationState.h"

#include "Emu/RISCV64Linux/RISCV64Info.h"
#include "Emu/RISCV64Linux/RISCV64OpInfo.h"
#include "Emu/RISCV64Linux/RISCV64Decoder.h"
#include "Emu/RISCV64Linux/RISCV64Operation.h"

using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::RISCV64Linux;
using namespace Onikiri::EmulatorUtility::Operation;
using namespace Onikiri::RISCV64Linux::Operation;

//
// 命令の定義
//

namespace {

    // 各命令形式に対するオペコードを得るためのマスク (0のビットが引数)
    static const u32 MASK_EXACT = 0xffffffff;  // 全bitが一致

    static const u32 MASK_LUI =     0x0000007f; // U-type, opcode
    static const u32 MASK_AUIPC =   0x0000007f; // U-type, opcode

    static const u32 MASK_IMM =     0x0000707f; // I-type, funct3 + opcode
    static const u32 MASK_SHIFT =   0xfc00707f; // I-type, funct3 + opcode
	static const u32 MASK_SHIFTW =  0xfe00707f; // I-type, funct3 + opcode
    static const u32 MASK_INT =     0xfe00707f; // R-type, funct7 + funct3 + opcode

	static const u32 MASK_CSR   =   0x0000707f; // I-type, funct3 + opcode

    static const u32 MASK_JAL =     0x0000007f; // J-type
    static const u32 MASK_J =       0x00000fff; // J-type, rd

    static const u32 MASK_JALR =    0x0000707f; // J-type?, 
    static const u32 MASK_CALLRET = 0x000fffff; // J-type, rs1, funct3, rd, opcode
    static const u32 MASK_CALL =    0x00007fff; // J-type, funct3, rd, opcode
    static const u32 MASK_RET =     0x000ff07f; // J-type, rs1, funct3, rd, opcode

    static const u32 MASK_BR  =     0x0000707f; // B-type, funct3

    static const u32 MASK_ST  =     0x0000707f; // B-type, funct3
    static const u32 MASK_LD  =     0x0000707f; // B-type, funct3

	static const u32 MASK_FLOAT  =  0xfe00707f; // R-type, funct7 + funct3 + opcode
	static const u32 MASK_SQRT   =  0xfff0707f; // R-type, funct7 + rs2(0) + funct3 + opcode
	static const u32 MASK_MULADD =  0x0600707f; // R4-type, funct2 + funct3 + opcode
	static const u32 MASK_FCVT   =  0xfff0707f; // R-type, funct7 + rs2 + funct3 + opcode
	static const u32 MASK_MV    =   0xfff0707f; // R-type, funct7 + rs2 + funct3 + opcode
	static const u32 MASK_FCLASS =  0xfff0707f; // R-type, funct7 + rs2 + funct3 + opcode



}

#define OPCODE_LUI()    0x37
#define OPCODE_AUIPC()  0x17

#define OPCODE_IMM(f) (u32)(((f) << 12) | 0x13)
#define OPCODE_SHIFT(f7, f3) (u32)(((f7) << 26) | ((f3) << 12) | 0x13) //変更
#define OPCODE_INT(f7, f3) (u32)(((f7) << 25) | ((f3) << 12) | 0x33)

#define OPCODE_JAL() (0x6f)
#define OPCODE_J()   (0x6f | (0 << 7))  // rd == x0

#define OPCODE_JALR()            (0x67 | (0 << 12))
#define OPCODE_CALLRET(rd, rs1)  (0x67 | (rd << 7) | (0 << 12) | (rs1 << 15))
#define OPCODE_CALL(rd)  (0x67 | (rd << 7) | (0 << 12))
#define OPCODE_RET(rs1)  (0x67 | (0 << 12) | (rs1 << 15))

#define OPCODE_BR(f)  (u32)(((f) << 12) | 0x63)

#define OPCODE_LD(f)  (u32)(((f) << 12) | 0x03)
#define OPCODE_ST(f)  (u32)(((f) << 12) | 0x23)

#define OPCODE_ECALL()  (u32)(0x73)
#define OPCODE_CSR(f)  (u32)(((f) << 12) | 0x73)

//new
#define OPCODE_ADDIW  0x1b
#define OPCODE_SHIFTW(f7, f3) (u32)(((f7) << 25) | ((f3) << 12) | 0x1b)
#define OPCODE_INTW(f7, f3) (u32)(((f7) << 25) | ((f3) << 12) | 0x3b)

#define OPCODE_FLD(f)  (u32)(((f) << 12) | 0x07)
#define OPCODE_FST(f)  (u32)(((f) << 12) | 0x27)

#define OPCODE_FLOAT(f7, f3) (u32)(((f7) << 25) | ((f3) << 12) | 0x53)
#define OPCODE_SQRT(f7, f3) (u32)(((f7) << 25) | (0 << 20) | ((f3) << 12) | 0x53)
#define OPCODE_MAX (u32)((0x14 << 25) | (1 << 12) | 0x53)
#define OPCODE_FMADD(fmt, f3) (u32)(((fmt) << 25) | ((f3) << 12) | 0x43)
#define OPCODE_FMSUB(fmt, f3) (u32)(((fmt) << 25) | ((f3) << 12) | 0x47)
#define OPCODE_FNMSUB(fmt, f3) (u32)(((fmt) << 25) | ((f3) << 12) | 0x4b)
#define OPCODE_FNMADD(fmt, f3) (u32)(((fmt) << 25) | ((f3) << 12) | 0x4f)
#define OPCODE_FCVT(f7, rs2, f3) (u32)(((f7) << 25) | (rs2 << 20) | ((f3) << 12) | 0x53)
#define OPCODE_MV(f7) (u32)(((f7) << 25) | (0 << 20) | (0 << 12) | 0x53)
#define OPCODE_FCLASS(f7) (u32)(((f7) << 25) | (0 << 20) | (1 << 12) | 0x53)



namespace {
    // オペランドのテンプレート
    // [RegTemplateBegin, RegTemplateEnd] は，命令中のオペランドレジスタ番号に置き換えられる
    // [ImmTemplateBegin, RegTemplateEnd] は，即値に置き換えられる

    // レジスタ・テンプレートに使用する番号
    // 本物のレジスタ番号を使ってはならない
    static const int RegTemplateBegin = -20;    // 命令中のレジスタ番号に変換 (数値に意味はない．本物のレジスタ番号と重ならずかつ一意であればよい)
    static const int RegTemplateEnd = RegTemplateBegin+4-1;
    static const int ImmTemplateBegin = -30;    // 即値に変換
    static const int ImmTemplateEnd = ImmTemplateBegin+2-1;

    const int R0 = RegTemplateBegin+0;
    const int R1 = RegTemplateBegin+1;
    const int R2 = RegTemplateBegin+2;
	const int R3 = RegTemplateBegin+3;


    const int I0 = ImmTemplateBegin+0;
    const int I1 = ImmTemplateBegin+1;

    const int T0 = RISCV64Info::REG_ADDRESS;
    const int FPC = RISCV64Info::REG_FPCR;
	 
}

#define RISCV64_DSTOP(n) RISCV64DstOperand<n>
#define RISCV64_SRCOP(n) RISCV64SrcOperand<n>
#define RISCV64_SRCOPFLOAT(n) Cast< float, AsFP< double, SrcOperand<n> > > //この方式怪しげ、実際は上位32bitに1を入れる？　全て修正対象
#define RISCV64_SRCOPDOUBLE(n) AsFP< double, SrcOperand<n> >

#define D0 RISCV64_DSTOP(0)
#define D1 RISCV64_DSTOP(1)
#define S0 RISCV64_SRCOP(0)
#define S1 RISCV64_SRCOP(1)
#define S2 RISCV64_SRCOP(2)
#define S3 RISCV64_SRCOP(3)
#define SF0 RISCV64_SRCOPFLOAT(0)
#define SF1 RISCV64_SRCOPFLOAT(1)
#define SF2 RISCV64_SRCOPFLOAT(2)
#define SF3 RISCV64_SRCOPFLOAT(3)
#define SD0 RISCV64_SRCOPDOUBLE(0)
#define SD1 RISCV64_SRCOPDOUBLE(1)
#define SD2 RISCV64_SRCOPDOUBLE(2)
#define SD3 RISCV64_SRCOPDOUBLE(3)

// 00000000 PAL 00 = halt
// 2ffe0000 ldq_u r31, r30(0) = unop
// 471f041f mslql r31, r31, r31 = nop

// no trap implemented

// 投機的にフェッチされたときにはエラーにせず，実行されたときにエラーにする
// syscallにすることにより，直前までの命令が完了してから実行される (実行は投機的でない)
RISCV64Converter::OpDef RISCV64Converter::m_OpDefUnknown = 
    {"unknown", MASK_EXACT, 0,  1, {{OpClassCode::UNDEF,    {-1, -1}, {I0, -1, -1, -1}, RISCV64Converter::RISCVUnknownOperation}}};


// branchは，OpInfo 列の最後じゃないとだめ
RISCV64Converter::OpDef RISCV64Converter::m_OpDefsBase[] = 
{
	//RV32I
    // LUI/AUIPC 変更
    //{Name,    Mask,       Opcode,         nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"lui",     MASK_LUI,   OPCODE_LUI(),   1,  { {OpClassCode::iALU,   {R0, -1},   {I0, -1, -1, -1},   Set<D0, RISCV64Lui<S0> >} } },
    {"auipc",   MASK_AUIPC, OPCODE_AUIPC(), 1,  { {OpClassCode::iALU,   {R0, -1},   {I0, -1, -1, -1},   Set<D0, RISCV64Auipc<S0> >} } },
    
    // IMM 
    //{Name,    Mask,       Opcode,         nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"addi",    MASK_IMM,   OPCODE_IMM(0),  1,  { {OpClassCode::iALU,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, IntAdd<u64, S0, S1> > } } },
    {"slti",    MASK_IMM,   OPCODE_IMM(2),  1,  { {OpClassCode::iALU,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, RISCV64Compare<S0, S1, IntCondLessSigned<u64> > > } } },
    {"sltiu",   MASK_IMM,   OPCODE_IMM(3),  1,  { {OpClassCode::iALU,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, RISCV64Compare<S0, S1, IntCondLessUnsigned<u64> > > } } },
    {"xori",    MASK_IMM,   OPCODE_IMM(4),  1,  { {OpClassCode::iALU,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, BitXor<u64, S0, S1> > } } },
    {"ori",     MASK_IMM,   OPCODE_IMM(6),  1,  { {OpClassCode::iALU,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, BitOr<u64, S0, S1> > } } },
    {"andi",    MASK_IMM,   OPCODE_IMM(7),  1,  { {OpClassCode::iALU,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, BitAnd<u64, S0, S1> > } } },
    
    // SHIFT　変更
    //{Name,    Mask,       Opcode,                 nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"slli",    MASK_SHIFT, OPCODE_SHIFT(0x00, 1),  1,  { {OpClassCode::iSFT,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, LShiftL<u64, S0, S1, 0x3f > > } } },
    {"srli",    MASK_SHIFT, OPCODE_SHIFT(0x00, 5),  1,  { {OpClassCode::iSFT,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, LShiftR<u64, S0, S1, 0x3f > > } } },
    {"srai",    MASK_SHIFT, OPCODE_SHIFT(0x10, 5),  1,  { {OpClassCode::iSFT,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, AShiftR<u64, S0, S1, 0x3f > > } } },

    // INT　SLL,SRL,SRA変更 
    //{Name,    Mask,       Opcode,                 nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"add",     MASK_INT,   OPCODE_INT(0x00, 0),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, IntAdd<u64, S0, S1> > } } },
    {"sub",     MASK_INT,   OPCODE_INT(0x20, 0),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, IntSub<u64, S0, S1> > } } },
    {"sll",     MASK_INT,   OPCODE_INT(0x00, 1),    1,  { {OpClassCode::iSFT,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, LShiftL<u64, S0, S1, 0x3f > > } } },
    {"slt",     MASK_INT,   OPCODE_INT(0x00, 2),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, RISCV64Compare<S0, S1, IntCondLessSigned<u64> > > } } },
    {"sltu",    MASK_INT,   OPCODE_INT(0x00, 3),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, RISCV64Compare<S0, S1, IntCondLessUnsigned<u64> > > } } },
    {"xor",     MASK_INT,   OPCODE_INT(0x00, 4),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, BitXor<u64, S0, S1> > } } },
    {"srl",     MASK_INT,   OPCODE_INT(0x00, 5),    1,  { {OpClassCode::iSFT,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, LShiftR<u64, S0, S1, 0x3f > > } } },
    {"sra",     MASK_INT,   OPCODE_INT(0x20, 5),    1,  { {OpClassCode::iSFT,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, AShiftR<u64, S0, S1, 0x3f > > } } },
    {"or",      MASK_INT,   OPCODE_INT(0x00, 6),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, BitOr<u64, S0, S1> > } } },
    {"and",     MASK_INT,   OPCODE_INT(0x00, 7),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, BitAnd<u64, S0, S1> > } } },

    // JAL
    // J must be placed before JAL, because MASK_JAL/OPCODE_JAL include J
    //{Name,    Mask,       Opcode,         nOp,{ OpClassCode,              Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"j",       MASK_J,     OPCODE_J(),     1,  { { OpClassCode::iJUMP,     {-1, -1},   {I0, -1, -1, -1},   RISCV64BranchRelUncond<S0> } } },
    {"jal",     MASK_JAL,   OPCODE_JAL(),   1,  { { OpClassCode::CALL_JUMP, {R0, -1},   {I0, -1, -1, -1},   RISCV64CallRelUncond<D0, S0> } } },
    
    // JALR
    // CALL/RET must be placed before JALR, because MASK_JALR/OPCODE_JALR include CALL/RET
    //    rd    rs1    rs1=rd   RAS action
    //    !link !link  -        none
    //    !link link   -        pop
    //    link  !link  -        push
    //    link  link   0        push and pop
    //    link  link   1        push
    //{Name,    Mask,           Opcode,                 nOp,{ OpClassCode,              Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"jalr",    MASK_CALLRET,   OPCODE_CALLRET(1, 5),   1,  { { OpClassCode::iJUMP,     {R0, -1},   {R1, I0, -1, -1},   RISCV64CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_CALLRET,   OPCODE_CALLRET(5, 1),   1,  { { OpClassCode::iJUMP,     {R0, -1},   {R1, I0, -1, -1},   RISCV64CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_CALL,      OPCODE_CALL(5),         1,  { { OpClassCode::CALL_JUMP, {R0, -1},   {R1, I0, -1, -1},   RISCV64CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_CALL,      OPCODE_CALL(1),         1,  { { OpClassCode::CALL_JUMP, {R0, -1},   {R1, I0, -1, -1},   RISCV64CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_RET,       OPCODE_RET(5),          1,  { { OpClassCode::RET,       {R0, -1},   {R1, I0, -1, -1},   RISCV64CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_RET,       OPCODE_RET(1),          1,  { { OpClassCode::RET,       {R0, -1},   {R1, I0, -1, -1},   RISCV64CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_CALL,      OPCODE_CALL(0),         1,  { { OpClassCode::iJUMP,     {-1, -1},   {R1, I0, -1, -1},   RISCV64CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_JALR,      OPCODE_JALR(),          1,  { { OpClassCode::CALL_JUMP, {R0, -1},   {R1, I0, -1, -1},   RISCV64CallAbsUncond<D0, S0, S1> } } },

    // Branch
    //{Name,    Mask,       Opcode,         nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"beq",     MASK_BR,    OPCODE_BR(0),   1,  { { OpClassCode::iBC,   {-1, -1},   {R0, R1, I0, -1},   RISCV64BranchRelCond<S2, Compare<S0, S1, IntCondEqual<u64> > > } } },
    {"bne",     MASK_BR,    OPCODE_BR(1),   1,  { { OpClassCode::iBC,   {-1, -1},   {R0, R1, I0, -1},   RISCV64BranchRelCond<S2, Compare<S0, S1, IntCondNotEqual<u64> > > } } },
    {"blt",     MASK_BR,    OPCODE_BR(4),   1,  { { OpClassCode::iBC,   {-1, -1},   {R0, R1, I0, -1},   RISCV64BranchRelCond<S2, Compare<S0, S1, IntCondLessSigned<u64> > > } } },
    {"bge",     MASK_BR,    OPCODE_BR(5),   1,  { { OpClassCode::iBC,   {-1, -1},   {R0, R1, I0, -1},   RISCV64BranchRelCond<S2, Compare<S0, S1, IntCondGreaterEqualSigned<u64> > > } } },
    {"bltu",    MASK_BR,    OPCODE_BR(6),   1,  { { OpClassCode::iBC,   {-1, -1},   {R0, R1, I0, -1},   RISCV64BranchRelCond<S2, Compare<S0, S1, IntCondLessUnsigned<u64> > > } } },
    {"bgeu",    MASK_BR,    OPCODE_BR(7),   1,  { { OpClassCode::iBC,   {-1, -1},   {R0, R1, I0, -1},   RISCV64BranchRelCond<S2, Compare<S0, S1, IntCondGreaterEqualUnsigned<u64> > > } } },

    // Store
    //{Name,    Mask,       Opcode,         nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"sb",      MASK_ST,    OPCODE_ST(0),   1,  { { OpClassCode::iST,   {-1, -1},   {R1, R0, I0, -1},   Store<u8, S0, RISCV64Addr<S1, S2> > } } },
    {"sh",      MASK_ST,    OPCODE_ST(1),   1,  { { OpClassCode::iST,   {-1, -1},   {R1, R0, I0, -1},   Store<u16, S0, RISCV64Addr<S1, S2> > } } },
    {"sw",      MASK_ST,    OPCODE_ST(2),   1,  { { OpClassCode::iST,   {-1, -1},   {R1, R0, I0, -1},   Store<u32, S0, RISCV64Addr<S1, S2> > } } },

    // Load　lw変更
    //{Name,    Mask,       Opcode,         nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"lb",      MASK_LD,    OPCODE_LD(0),   1,  { { OpClassCode::iLD,   {R0, -1},   {R1, I0, -1, -1},   SetSext<D0, Load<u8,  RISCV64Addr<S0, S1> > > } } },
    {"lh",      MASK_LD,    OPCODE_LD(1),   1,  { { OpClassCode::iLD,   {R0, -1},   {R1, I0, -1, -1},   SetSext<D0, Load<u16, RISCV64Addr<S0, S1> > > } } },
    {"lw",      MASK_LD,    OPCODE_LD(2),   1,  { { OpClassCode::iLD,   {R0, -1},   {R1, I0, -1, -1},   SetSext<D0, Load<u32, RISCV64Addr<S0, S1> > > } } },
    {"lbu",     MASK_LD,    OPCODE_LD(4),   1,  { { OpClassCode::iLD,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, Load<u8,  RISCV64Addr<S0, S1> > > } } },
    {"lhu",     MASK_LD,    OPCODE_LD(5),   1,  { { OpClassCode::iLD,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, Load<u16, RISCV64Addr<S0, S1> > > } } },

    // System
    //{Name,    Mask,       Opcode,         nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"ecall",   MASK_EXACT, OPCODE_ECALL(),   2,  {
        {OpClassCode::syscall,          {17, -1}, {17, 10, 11, -1}, RISCV64SyscallSetArg} ,
        {OpClassCode::syscall_branch,   {10, -1}, {17, 12, 13, -1}, RISCV64SyscallCore},
    }},

	// Csr
	//{Name,    Mask,       Opcode,          nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
	{"csrrw",   MASK_CSR,   OPCODE_CSR(1),   2,  {
        {OpClassCode::syscall,          {-1, -1}, {-1, -1, -1, -1}, RISCV64SyscallSetArg} ,
        {OpClassCode::syscall_branch,   {R0, R2}, {R1, R2, -1, -1}, RISCV64CSRRW<D0, D1, S0, S1>},
    }},
	{"csrrs",   MASK_CSR,   OPCODE_CSR(2),   2,  {
        {OpClassCode::syscall,          {-1, -1}, {-1, -1, -1, -1}, RISCV64SyscallSetArg} ,
        {OpClassCode::syscall_branch,   {R0, R2}, {R1, R2, -1, -1}, RISCV64CSRRS<D0, D1, S0, S1>},
    }},
	{"csrrc",   MASK_CSR,   OPCODE_CSR(3),   2,  {
        {OpClassCode::syscall,          {-1, -1}, {-1, -1, -1, -1}, RISCV64SyscallSetArg} ,
		{OpClassCode::syscall_branch,   {R0, R2}, {R1, R2, -1, -1}, RISCV64CSRRC<D0, D1, S0, S1>},
    }},
	{"csrrwi",   MASK_CSR,   OPCODE_CSR(5),   2,  {
        {OpClassCode::syscall,          {-1, -1}, {-1, -1, -1, -1}, RISCV64SyscallSetArg} ,
        {OpClassCode::syscall_branch,   {R0, R1}, {I0, R1, -1, -1}, RISCV64CSRRW<D0, D1, S0, S1>},
    }},
	{"csrrsi",   MASK_CSR,   OPCODE_CSR(6),   2,  {
        {OpClassCode::syscall,          {-1, -1}, {-1, -1, -1, -1}, RISCV64SyscallSetArg} ,
        {OpClassCode::syscall_branch,   {R0, R1}, {I0, R1, -1, -1}, RISCV64CSRRS<D0, D1, S0, S1>},
    }},
	{"csrrci",   MASK_CSR,   OPCODE_CSR(7),   2,  {
        {OpClassCode::syscall,          {-1, -1}, {-1, -1, -1, -1}, RISCV64SyscallSetArg} ,
        {OpClassCode::syscall_branch,   {R0, R1}, {I0, R1, -1, -1}, RISCV64CSRRC<D0, D1, S0, S1>},
    }},
	

    // RV32M
    //{Name,    Mask,       Opcode,                 nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"mul",     MASK_INT,   OPCODE_INT(0x01, 0),    1,  { {OpClassCode::iMUL,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, IntMul<u64, S0, S1> > } } },
    {"mulh",    MASK_INT,   OPCODE_INT(0x01, 1),    1,  { {OpClassCode::iMUL,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, IntSMulh64<S0, S1> > } } },
    {"mulhus",  MASK_INT,   OPCODE_INT(0x01, 2),    1,  { {OpClassCode::iMUL,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, IntSUMulh64<S0, S1> > } } },
    {"mulhu",   MASK_INT,   OPCODE_INT(0x01, 3),    1,  { {OpClassCode::iMUL,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, IntUMulh64<S0, S1> > } } },
    {"div",     MASK_INT,   OPCODE_INT(0x01, 4),    1,  { {OpClassCode::iDIV,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, RISCV64IntDiv<S0, S1> > } } },
    {"divu",    MASK_INT,   OPCODE_INT(0x01, 5),    1,  { {OpClassCode::iDIV,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, RISCV64IntDivu<S0, S1> > } } },
    {"rem",     MASK_INT,   OPCODE_INT(0x01, 6),    1,  { {OpClassCode::iDIV,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, RISCV64IntRem<S0, S1> > } } },
    {"remu",    MASK_INT,   OPCODE_INT(0x01, 7),    1,  { {OpClassCode::iDIV,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, RISCV64IntRemu<S0, S1> > } } },

	//RV64I
	//IMM
	//{Name,    Mask,       Opcode,         nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
	{ "addiw",  MASK_IMM,   OPCODE_ADDIW,   1,{ { OpClassCode::iALU,   {R0, -1},   {R1, I0, -1, -1},   SetSext<D0, IntAdd<u32, S0, S1> > } } },

	//SHIFT
	//{Name,    Mask,       Opcode,                  nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
	{ "slliw",  MASK_SHIFT, OPCODE_SHIFTW(0x00, 1),  1,{ { OpClassCode::iSFT,   {R0, -1},   {R1, I0, -1, -1},   SetSext<D0, LShiftL<u32, S0, S1, 0x1f > > } } },
	{ "srliw",  MASK_SHIFT, OPCODE_SHIFTW(0x00, 5),  1,{ { OpClassCode::iSFT,   {R0, -1},   {R1, I0, -1, -1},   SetSext<D0, LShiftR<u32, S0, S1, 0x1f > > } } },
	{ "sraiw",  MASK_SHIFT, OPCODE_SHIFTW(0x20, 5),  1,{ { OpClassCode::iSFT,   {R0, -1},   {R1, I0, -1, -1},   SetSext<D0, AShiftR<u32, S0, S1, 0x1f > > } } },

	// INT
	//{Name,    Mask,       Opcode,                  nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
	{ "addw",   MASK_INT,   OPCODE_INTW(0x00, 0),    1,{ { OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   SetSext<D0, IntAdd<u32, S0, S1> > } } },
	{ "subw",   MASK_INT,   OPCODE_INTW(0x20, 0),    1,{ { OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   SetSext<D0, IntSub<u32, S0, S1> > } } },
	{ "sllw",   MASK_INT,   OPCODE_INTW(0x00, 1),    1,{ { OpClassCode::iSFT,   {R0, -1},   {R1, R2, -1, -1},   SetSext<D0, LShiftL<u32, S0, S1, 0x1f > > } } },
	{ "srlw",   MASK_INT,   OPCODE_INTW(0x00, 5),    1,{ { OpClassCode::iSFT,   {R0, -1},   {R1, R2, -1, -1},   SetSext<D0, LShiftR<u32, S0, S1, 0x1f > > } } },
	{ "sraw",   MASK_INT,   OPCODE_INTW(0x20, 5),    1,{ { OpClassCode::iSFT,   {R0, -1},   {R1, R2, -1, -1},   SetSext<D0, AShiftR<u32, S0, S1, 0x1f > > } } },
	
	//LOAD/STORE
	//{Name,    Mask,       Opcode,         nOp,{ OpClassCode,        Dst[],      Src[],               OpInfoType::EmulationFunc}[]}
	{ "ld",     MASK_LD,    OPCODE_LD(3),   1,{ { OpClassCode::iLD,   {R0, -1},   {R1, I0, -1, -1},    Set<D0, Load<u64, RISCV64Addr<S0, S1> > > } } },
	{ "lwu",    MASK_LD,    OPCODE_LD(6),   1,{ { OpClassCode::iLD,   {R0, -1},   {R1, I0, -1, -1},    Set<D0, Load<u32, RISCV64Addr<S0, S1> > > } } },
	{ "sd",     MASK_ST,    OPCODE_ST(3),   1,{ { OpClassCode::iST,   {-1, -1},   {R1, R0, I0, -1},    Store<u64, S0, RISCV64Addr<S1, S2> > } } },


	//RV64M 
	//{Name,    Mask,       Opcode,                  nOp,{ OpClassCode,         Dst[],       Src[],               OpInfoType::EmulationFunc}[]}
	{ "mulw",   MASK_INT,   OPCODE_INTW(0x01, 0),    1,{ { OpClassCode::iMUL,   {R0, -1},    {R1, R2, -1, -1},    SetSext<D0, IntMul<u32, S0, S1> > } } },
	{ "divw",   MASK_INT,   OPCODE_INTW(0x01, 4),    1,{ { OpClassCode::iDIV,   {R0, -1},    {R1, R2, -1, -1},    SetSext<D0, RISCV64IntDivw<S0, S1> > } } },
	{ "divuw",  MASK_INT,   OPCODE_INTW(0x01, 5),    1,{ { OpClassCode::iDIV,   {R0, -1},    {R1, R2, -1, -1},    SetSext<D0, RISCV64IntDivuw<S0, S1> > } } },
	{ "remw",   MASK_INT,   OPCODE_INTW(0x01, 6),    1,{ { OpClassCode::iDIV,   {R0, -1},    {R1, R2, -1, -1},    SetSext<D0, RISCV64IntRemw<S0, S1> > } } },
	{ "remuw",  MASK_INT,   OPCODE_INTW(0x01, 7),    1,{ { OpClassCode::iDIV,   {R0, -1},    {R1, R2, -1, -1},    SetSext<D0, RISCV64IntRemuw<S0, S1> > } } },


	//RV32F
	//LOAD/STORE
	//{Name,    Mask,       Opcode,           nOp,{ OpClassCode,        Dst[],      Src[],               OpInfoType::EmulationFunc}[]}
    { "flw",    MASK_LD,    OPCODE_FLD(2),    1,{ { OpClassCode::fLD,   {R0, -1},   {R1, I0, -1, -1},    Set<D0, Load<u32, RISCV64Addr<S0, S1> > > } } },
	{ "fsw",    MASK_ST,    OPCODE_FST(2),    1,{ { OpClassCode::fST,   {-1, -1},   {R1, R0, I0, -1},    Store<u32, S0, RISCV64Addr<S1, S2> > } } },

	//FLOAT
	//rneとrmmの違いがよくわかってません
	//{Name,            Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
	{ "fadd.s/rne",     MASK_FLOAT,   OPCODE_FLOAT(0x00, 0),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPAdd< f32, SF0, SF1, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fadd.s/rtz",     MASK_FLOAT,   OPCODE_FLOAT(0x00, 1),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPAdd< f32, SF0, SF1, IntConst<int, FE_TOWARDZERO> > > > } } },
	{ "fadd.s/rdn",     MASK_FLOAT,   OPCODE_FLOAT(0x00, 2),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPAdd< f32, SF0, SF1, IntConst<int, FE_DOWNWARD> > > > } } },
	{ "fadd.s/rup",     MASK_FLOAT,   OPCODE_FLOAT(0x00, 3),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPAdd< f32, SF0, SF1, IntConst<int, FE_UPWARD> > > > } } },
	{ "fadd.s/rmm",     MASK_FLOAT,   OPCODE_FLOAT(0x00, 4),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPAdd< f32, SF0, SF1, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fsub.s/rne",     MASK_FLOAT,   OPCODE_FLOAT(0x04, 0),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPSub< f32, SF0, SF1, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fsub.s/rtz",     MASK_FLOAT,   OPCODE_FLOAT(0x04, 1),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPSub< f32, SF0, SF1, IntConst<int, FE_TOWARDZERO> > > > } } },
	{ "fsub.s/rdn",     MASK_FLOAT,   OPCODE_FLOAT(0x04, 2),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPSub< f32, SF0, SF1, IntConst<int, FE_DOWNWARD> > > > } } },
	{ "fsub.s/rup",     MASK_FLOAT,   OPCODE_FLOAT(0x04, 3),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPSub< f32, SF0, SF1, IntConst<int, FE_UPWARD> > > > } } },
	{ "fsub.s/rmm",     MASK_FLOAT,   OPCODE_FLOAT(0x04, 4),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPSub< f32, SF0, SF1, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fmul.s/rne",     MASK_FLOAT,   OPCODE_FLOAT(0x08, 0),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPMul< f32, SF0, SF1, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fmul.s/rtz",     MASK_FLOAT,   OPCODE_FLOAT(0x08, 1),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPMul< f32, SF0, SF1, IntConst<int, FE_TOWARDZERO> > > > } } },
	{ "fmul.s/rdn",     MASK_FLOAT,   OPCODE_FLOAT(0x08, 2),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPMul< f32, SF0, SF1, IntConst<int, FE_DOWNWARD> > > > } } },
	{ "fmul.s/rup",     MASK_FLOAT,   OPCODE_FLOAT(0x08, 3),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPMul< f32, SF0, SF1, IntConst<int, FE_UPWARD> > > > } } },
	{ "fmul.s/rmm",     MASK_FLOAT,   OPCODE_FLOAT(0x08, 4),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPMul< f32, SF0, SF1, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fdiv.s/rne",     MASK_FLOAT,   OPCODE_FLOAT(0x0c, 0),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPDiv< f32, SF0, SF1, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fdiv.s/rtz",     MASK_FLOAT,   OPCODE_FLOAT(0x0c, 1),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPDiv< f32, SF0, SF1, IntConst<int, FE_TOWARDZERO> > > > } } },
	{ "fdiv.s/rdn",     MASK_FLOAT,   OPCODE_FLOAT(0x0c, 2),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPDiv< f32, SF0, SF1, IntConst<int, FE_DOWNWARD> > > > } } },
	{ "fdiv.s/rup",     MASK_FLOAT,   OPCODE_FLOAT(0x0c, 3),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPDiv< f32, SF0, SF1, IntConst<int, FE_UPWARD> > > > } } },
	{ "fdiv.s/rmm",     MASK_FLOAT,   OPCODE_FLOAT(0x0c, 4),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, FPDiv< f32, SF0, SF1, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fsqrt.s/rne",    MASK_SQRT,    OPCODE_SQRT(0x2c, 0),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast<f64, FPSqrt< f32, SF0, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fsqrt.s/rtz",    MASK_SQRT,    OPCODE_SQRT(0x2c, 1),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast<f64, FPSqrt< f32, SF0, IntConst<int, FE_TOWARDZERO> > > > } } },
	{ "fsqrt.s/rdn",    MASK_SQRT,    OPCODE_SQRT(0x2c, 2),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast<f64, FPSqrt< f32, SF0, IntConst<int, FE_DOWNWARD> > > > } } },
	{ "fsqrt.s/rup",    MASK_SQRT,    OPCODE_SQRT(0x2c, 3),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast<f64, FPSqrt< f32, SF0, IntConst<int, FE_UPWARD> > > > } } },
	{ "fsqrt.s/rmm",    MASK_SQRT,    OPCODE_SQRT(0x2c, 4),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast<f64, FPSqrt< f32, SF0, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fmin.s",         MASK_FLOAT,   OPCODE_FLOAT(0x14, 0),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, RISCV64MIN< f32, SF0, SF1> > > } } },
	{ "fmax.s",         MASK_FLOAT,   OPCODE_FLOAT(0x14, 1),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, Cast<f64, RISCV64MAX< f32, SF0, SF1> > > } } },
	{ "fmadd.s/rne",    MASK_MULADD,  OPCODE_FMADD(0, 0),       1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64MADD< f32, SF0, SF1, SF2, IntConst<int, FE_TONEAREST> > > > } } }, //fADD?
	{ "fmadd.s/rtz",    MASK_MULADD,  OPCODE_FMADD(0, 1),       1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64MADD< f32, SF0, SF1, SF2, IntConst<int, FE_TOWARDZERO> > > > } } },
	{ "fmadd.s/rdn",    MASK_MULADD,  OPCODE_FMADD(0, 2),       1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64MADD< f32, SF0, SF1, SF2, IntConst<int, FE_DOWNWARD> > > > } } },
	{ "fmadd.s/rup",    MASK_MULADD,  OPCODE_FMADD(0, 3),       1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64MADD< f32, SF0, SF1, SF2, IntConst<int, FE_UPWARD> > > > } } },
	{ "fmadd.s/rmm",    MASK_MULADD,  OPCODE_FMADD(0, 4),       1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64MADD< f32, SF0, SF1, SF2, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fmsub.s/rne",    MASK_MULADD,  OPCODE_FMSUB(0, 0),       1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64MSUB< f32, SF0, SF1, SF2, IntConst<int, FE_TONEAREST> > > > } } }, 
	{ "fmsub.s/rtz",    MASK_MULADD,  OPCODE_FMSUB(0, 1),       1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64MSUB< f32, SF0, SF1, SF2, IntConst<int, FE_TOWARDZERO> > > > } } },
	{ "fmsub.s/rdn",    MASK_MULADD,  OPCODE_FMSUB(0, 2),       1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64MSUB< f32, SF0, SF1, SF2, IntConst<int, FE_DOWNWARD> > > > } } },
	{ "fmsub.s/rup",    MASK_MULADD,  OPCODE_FMSUB(0, 3),       1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64MSUB< f32, SF0, SF1, SF2, IntConst<int, FE_UPWARD> > > > } } },
	{ "fmsub.s/rmm",    MASK_MULADD,  OPCODE_FMSUB(0, 4),       1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64MSUB< f32, SF0, SF1, SF2, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fnmadd.s/rne",   MASK_MULADD,  OPCODE_FNMADD(0, 0),      1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64NMADD< f32, SF0, SF1, SF2, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fnmadd.s/rtz",   MASK_MULADD,  OPCODE_FNMADD(0, 1),      1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64NMADD< f32, SF0, SF1, SF2, IntConst<int, FE_TOWARDZERO> > > > } } },
	{ "fnmadd.s/rdn",   MASK_MULADD,  OPCODE_FNMADD(0, 2),      1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64NMADD< f32, SF0, SF1, SF2, IntConst<int, FE_DOWNWARD> > > > } } },
	{ "fnmadd.s/rup",   MASK_MULADD,  OPCODE_FNMADD(0, 3),      1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64NMADD< f32, SF0, SF1, SF2, IntConst<int, FE_UPWARD> > > > } } },
	{ "fnmadd.s/rmm",   MASK_MULADD,  OPCODE_FNMADD(0, 4),      1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64NMADD< f32, SF0, SF1, SF2, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fnmsub.s/rne",   MASK_MULADD,  OPCODE_FNMSUB(0, 0),      1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64NMSUB< f32, SF0, SF1, SF2, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fnmsub.s/rtz",   MASK_MULADD,  OPCODE_FNMSUB(0, 1),      1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64NMSUB< f32, SF0, SF1, SF2, IntConst<int, FE_TOWARDZERO> > > > } } },
	{ "fnmsub.s/rdn",   MASK_MULADD,  OPCODE_FNMSUB(0, 2),      1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64NMSUB< f32, SF0, SF1, SF2, IntConst<int, FE_DOWNWARD> > > > } } },
	{ "fnmsub.s/rup",   MASK_MULADD,  OPCODE_FNMSUB(0, 3),      1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64NMSUB< f32, SF0, SF1, SF2, IntConst<int, FE_UPWARD> > > > } } },
    { "fnmsub.s/rmm",   MASK_MULADD,  OPCODE_FNMSUB(0, 4),      1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, Cast<f64, RISCV64NMSUB< f32, SF0, SF1, SF2, IntConst<int, FE_TONEAREST> > > > } } },


	//conversion
	//{Name,            Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
	{ "fcvt.w.s/rne",   MASK_FCVT,    OPCODE_FCVT(0x60, 0, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetSext< D0, RISCV64FPToInt< s32, SF0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.w.s/rtz",   MASK_FCVT,    OPCODE_FCVT(0x60, 0, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetSext< D0, RISCV64FPToInt< s32, SF0, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fcvt.w.s/rdn",   MASK_FCVT,    OPCODE_FCVT(0x60, 0, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetSext< D0, RISCV64FPToInt< s32, SF0, IntConst<int, FE_DOWNWARD> > > } } },
    { "fcvt.w.s/rup",   MASK_FCVT,    OPCODE_FCVT(0x60, 0, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetSext< D0, RISCV64FPToInt< s32, SF0, IntConst<int, FE_UPWARD> > > } } },
	{ "fcvt.w.s/rmm",   MASK_FCVT,    OPCODE_FCVT(0x60, 0, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetSext< D0, RISCV64FPToInt< s32, SF0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.wu.s/rne",  MASK_FCVT,    OPCODE_FCVT(0x60, 1, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u32, SF0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.wu.s/rtz",  MASK_FCVT,    OPCODE_FCVT(0x60, 1, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u32, SF0, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fcvt.wu.s/rdn",  MASK_FCVT,    OPCODE_FCVT(0x60, 1, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u32, SF0, IntConst<int, FE_DOWNWARD> > > } } },
    { "fcvt.wu.s/rup",  MASK_FCVT,    OPCODE_FCVT(0x60, 1, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u32, SF0, IntConst<int, FE_UPWARD> > > } } },
	{ "fcvt.wu.s/rmm",  MASK_FCVT,    OPCODE_FCVT(0x60, 1, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u32, SF0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.s.w/rne",   MASK_FCVT,    OPCODE_FCVT(0x68, 0, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< s32, S0>, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fcvt.s.w/rtz",   MASK_FCVT,    OPCODE_FCVT(0x68, 0, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< s32, S0>, IntConst<int, FE_TOWARDZERO> > > > } } },
	{ "fcvt.s.w/rdn",   MASK_FCVT,    OPCODE_FCVT(0x68, 0, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< s32, S0>, IntConst<int, FE_DOWNWARD> > > > } } },
    { "fcvt.s.w/rup",   MASK_FCVT,    OPCODE_FCVT(0x68, 0, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< s32, S0>, IntConst<int, FE_UPWARD> > > > } } },
	{ "fcvt.s.w/rmm",   MASK_FCVT,    OPCODE_FCVT(0x68, 0, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< s32, S0>, IntConst<int, FE_TONEAREST> > > > } } },
    { "fcvt.s.wu/rne",  MASK_FCVT,    OPCODE_FCVT(0x68, 1, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< u32, S0>, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fcvt.s.wu/rtz",  MASK_FCVT,    OPCODE_FCVT(0x68, 1, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< u32, S0>, IntConst<int, FE_TOWARDZERO> > > > } } },
	{ "fcvt.s.wu/rdn",  MASK_FCVT,    OPCODE_FCVT(0x68, 1, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< u32, S0>, IntConst<int, FE_DOWNWARD> > > > } } },
    { "fcvt.s.wu/rup",  MASK_FCVT,    OPCODE_FCVT(0x68, 1, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< u32, S0>, IntConst<int, FE_UPWARD> > > > } } },
	{ "fcvt.s.wu/rmm",  MASK_FCVT,    OPCODE_FCVT(0x68, 1, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< u32, S0>, IntConst<int, FE_TONEAREST> > > > } } },

	//move
	//{Name,        Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
	{ "fsgnj.s",    MASK_FLOAT,   OPCODE_FLOAT(0x10, 0),    1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, R2, -1, -1},   SetSext< D0, FPFLOATCopySign<S1, S0> > } } },
	{ "fsgnjn.s",   MASK_FLOAT,   OPCODE_FLOAT(0x10, 1),    1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, R2, -1, -1},   SetSext< D0, FPFLOATCopySignNeg<S1, S0> > } } },
	{ "fsgnjx.s",   MASK_FLOAT,   OPCODE_FLOAT(0x10, 2),    1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, R2, -1, -1},   SetSext< D0, FPFLOATCopySignXor<S1, S0> > } } },
	{ "fmv.x.w",    MASK_MV,      OPCODE_MV(0x70),          1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, -1, -1, -1},   SetSext< D0, AsInt<s32, SF0> > } } },
	{ "fmv.w.x",    MASK_MV,      OPCODE_MV(0x78),          1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, -1, -1, -1},   SetSext< D0, Cast< s32, S0> > } } }, //これでいいのか？

	//compare
	//{Name,        Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
	{ "feq.s",      MASK_FLOAT,   OPCODE_FLOAT(0x50, 2),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV64FPEqual< f32, SF0, SF1> > } } },
	{ "flt.s",      MASK_FLOAT,   OPCODE_FLOAT(0x50, 1),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV64FPLess< f32, SF0, SF1> > } } },
	{ "fle.s",      MASK_FLOAT,   OPCODE_FLOAT(0x50, 0),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV64FPLessEqual< f32, SF0, SF1> > } } },

	//class
	//{Name,        Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
	{ "fclass.s",   MASK_FCLASS,  OPCODE_FCLASS(0x70),      1,{ { OpClassCode::fADD,   {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FCLASS< f32, SF0> > } } },


	//RV64F
	//{Name,            Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
	{ "fcvt.l.s/rne",   MASK_FCVT,    OPCODE_FCVT(0x60, 2, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToInt< s64, SF0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.l.s/rtz",   MASK_FCVT,    OPCODE_FCVT(0x60, 2, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToInt< s64, SF0, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fcvt.l.s/rdn",   MASK_FCVT,    OPCODE_FCVT(0x60, 2, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToInt< s64, SF0, IntConst<int, FE_DOWNWARD> > > } } },
	{ "fcvt.l.s/rup",   MASK_FCVT,    OPCODE_FCVT(0x60, 2, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToInt< s64, SF0, IntConst<int, FE_UPWARD> > > } } },
	{ "fcvt.l.s/rmm",   MASK_FCVT,    OPCODE_FCVT(0x60, 2, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToInt< s64, SF0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.lu.s/rne",  MASK_FCVT,    OPCODE_FCVT(0x60, 3, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u64, SF0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.lu.s/rtz",  MASK_FCVT,    OPCODE_FCVT(0x60, 3, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u64, SF0, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fcvt.lu.s/rdn",  MASK_FCVT,    OPCODE_FCVT(0x60, 3, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u64, SF0, IntConst<int, FE_DOWNWARD> > > } } },
	{ "fcvt.lu.s/rup",  MASK_FCVT,    OPCODE_FCVT(0x60, 3, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u64, SF0, IntConst<int, FE_UPWARD> > > } } },
	{ "fcvt.lu.s/rmm",  MASK_FCVT,    OPCODE_FCVT(0x60, 3, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u64, SF0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.s.l/rne",   MASK_FCVT,    OPCODE_FCVT(0x68, 2, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< s64, S0>, IntConst<int, FE_TONEAREST> > > > } } }, //これでいいの？
	{ "fcvt.s.l/rtz",   MASK_FCVT,    OPCODE_FCVT(0x68, 2, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< s64, S0>, IntConst<int, FE_TOWARDZERO> > > > } } },
	{ "fcvt.s.l/rdn",   MASK_FCVT,    OPCODE_FCVT(0x68, 2, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< s64, S0>, IntConst<int, FE_DOWNWARD> > > > } } },
	{ "fcvt.s.l/rup",   MASK_FCVT,    OPCODE_FCVT(0x68, 2, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< s64, S0>, IntConst<int, FE_UPWARD> > > > } } },
	{ "fcvt.s.l/rmm",   MASK_FCVT,    OPCODE_FCVT(0x68, 2, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< s64, S0>, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fcvt.s.lu/rne",  MASK_FCVT,    OPCODE_FCVT(0x68, 3, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< u64, S0>, IntConst<int, FE_TONEAREST> > > > } } },
	{ "fcvt.s.lu/rtz",  MASK_FCVT,    OPCODE_FCVT(0x68, 3, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< u64, S0>, IntConst<int, FE_TOWARDZERO> > > > } } },
	{ "fcvt.s.lu/rdn",  MASK_FCVT,    OPCODE_FCVT(0x68, 3, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< u64, S0>, IntConst<int, FE_DOWNWARD> > > > } } },
	{ "fcvt.s.lu/rup",  MASK_FCVT,    OPCODE_FCVT(0x68, 3, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< u64, S0>, IntConst<int, FE_UPWARD> > > > } } },
	{ "fcvt.s.lu/rmm",  MASK_FCVT,    OPCODE_FCVT(0x68, 3, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, Cast< f64, CastFP< f32, Cast< u64, S0>, IntConst<int, FE_TONEAREST> > > > } } },

    //RV32D

	//LOAD/STORE
	//{Name,    Mask,       Opcode,           nOp,{ OpClassCode,        Dst[],      Src[],               OpInfoType::EmulationFunc}[]}
    { "fld",    MASK_LD,    OPCODE_FLD(3),    1,{ { OpClassCode::fLD,   {R0, -1},   {R1, I0, -1, -1},    Set<D0, Load<u64, RISCV64Addr<S0, S1> > > } } },
	{ "fsd",    MASK_ST,    OPCODE_FST(3),    1,{ { OpClassCode::fST,   {-1, -1},   {R1, R0, I0, -1},    Store<u64, S0, RISCV64Addr<S1, S2> > } } },

	//FLOAT
	//rneとrmmの違いがよくわかってません
	//{Name,            Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
	{ "fadd.d/rne",     MASK_FLOAT,   OPCODE_FLOAT(0x01, 0),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPAdd< f64, SD0, SD1, IntConst<int, FE_TONEAREST> > > } } },
	{ "fadd.d/rtz",     MASK_FLOAT,   OPCODE_FLOAT(0x01, 1),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPAdd< f64, SD0, SD1, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fadd.d/rdn",     MASK_FLOAT,   OPCODE_FLOAT(0x01, 2),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPAdd< f64, SD0, SD1, IntConst<int, FE_DOWNWARD> > > } } },
	{ "fadd.d/rup",     MASK_FLOAT,   OPCODE_FLOAT(0x01, 3),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPAdd< f64, SD0, SD1, IntConst<int, FE_UPWARD> > > } } },
	{ "fadd.d/rmm",     MASK_FLOAT,   OPCODE_FLOAT(0x01, 4),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPAdd< f64, SD0, SD1, IntConst<int, FE_TONEAREST> > > } } },
	{ "fsub.d/rne",     MASK_FLOAT,   OPCODE_FLOAT(0x05, 0),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPSub< f64, SD0, SD1, IntConst<int, FE_TONEAREST> > > } } },
	{ "fsub.d/rtz",     MASK_FLOAT,   OPCODE_FLOAT(0x05, 1),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPSub< f64, SD0, SD1, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fsub.d/rdn",     MASK_FLOAT,   OPCODE_FLOAT(0x05, 2),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPSub< f64, SD0, SD1, IntConst<int, FE_DOWNWARD> > > } } },
	{ "fsub.d/rup",     MASK_FLOAT,   OPCODE_FLOAT(0x05, 3),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPSub< f64, SD0, SD1, IntConst<int, FE_UPWARD> > > } } },
	{ "fsub.d/rmm",     MASK_FLOAT,   OPCODE_FLOAT(0x05, 4),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPSub< f64, SD0, SD1, IntConst<int, FE_TONEAREST> > > } } },
	{ "fmul.d/rne",     MASK_FLOAT,   OPCODE_FLOAT(0x09, 0),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPMul< f64, SD0, SD1, IntConst<int, FE_TONEAREST> > > } } },
	{ "fmul.d/rtz",     MASK_FLOAT,   OPCODE_FLOAT(0x09, 1),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPMul< f64, SD0, SD1, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fmul.d/rdn",     MASK_FLOAT,   OPCODE_FLOAT(0x09, 2),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPMul< f64, SD0, SD1, IntConst<int, FE_DOWNWARD> > > } } },
	{ "fmul.d/rup",     MASK_FLOAT,   OPCODE_FLOAT(0x09, 3),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPMul< f64, SD0, SD1, IntConst<int, FE_UPWARD> > > } } },
	{ "fmul.d/rmm",     MASK_FLOAT,   OPCODE_FLOAT(0x09, 4),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPMul< f64, SD0, SD1, IntConst<int, FE_TONEAREST> > > } } },
	{ "fdiv.d/rne",     MASK_FLOAT,   OPCODE_FLOAT(0x0d, 0),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPDiv< f64, SD0, SD1, IntConst<int, FE_TONEAREST> > > } } },
	{ "fdiv.d/rtz",     MASK_FLOAT,   OPCODE_FLOAT(0x0d, 1),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPDiv< f64, SD0, SD1, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fdiv.d/rdn",     MASK_FLOAT,   OPCODE_FLOAT(0x0d, 2),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPDiv< f64, SD0, SD1, IntConst<int, FE_DOWNWARD> > > } } },
	{ "fdiv.d/rup",     MASK_FLOAT,   OPCODE_FLOAT(0x0d, 3),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPDiv< f64, SD0, SD1, IntConst<int, FE_UPWARD> > > } } },
	{ "fdiv.d/rmm",     MASK_FLOAT,   OPCODE_FLOAT(0x0d, 4),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPDiv< f64, SD0, SD1, IntConst<int, FE_TONEAREST> > > } } },
	{ "fsqrt.d/rne",    MASK_SQRT,    OPCODE_SQRT(0x2d, 0),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, FPSqrt< f64, SD0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fsqrt.d/rtz",    MASK_SQRT,    OPCODE_SQRT(0x2d, 1),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, FPSqrt< f64, SD0, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fsqrt.d/rdn",    MASK_SQRT,    OPCODE_SQRT(0x2d, 2),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, FPSqrt< f64, SD0, IntConst<int, FE_DOWNWARD> > > } } },
	{ "fsqrt.d/rup",    MASK_SQRT,    OPCODE_SQRT(0x2d, 3),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, FPSqrt< f64, SD0, IntConst<int, FE_UPWARD> > > } } },
	{ "fsqrt.d/rmm",    MASK_SQRT,    OPCODE_SQRT(0x2d, 4),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, FPSqrt< f64, SD0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fmin.d",         MASK_FLOAT,   OPCODE_FLOAT(0x15, 0),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, RISCV64MIN< f64, SD0, SD1> > } } },
	{ "fmax.d",         MASK_FLOAT,   OPCODE_FLOAT(0x15, 1),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, RISCV64MAX< f64, SD0, SD1> > } } },
	{ "fmadd.d/rtz",    MASK_MULADD,  OPCODE_FMADD(1, 1),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64MADD< f64, SD0, SD1, SD2, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fmadd.d/rdn",    MASK_MULADD,  OPCODE_FMADD(1, 2),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64MADD< f64, SD0, SD1, SD2, IntConst<int, FE_DOWNWARD> > > } } },
	{ "fmadd.d/rne",    MASK_MULADD,  OPCODE_FMADD(1, 0),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64MADD< f64, SD0, SD1, SD2, IntConst<int, FE_TONEAREST> > > } } }, //fADD?
	{ "fmadd.d/rup",    MASK_MULADD,  OPCODE_FMADD(1, 3),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64MADD< f64, SD0, SD1, SD2, IntConst<int, FE_UPWARD> > > } } },
	{ "fmadd.d/rmm",    MASK_MULADD,  OPCODE_FMADD(1, 4),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64MADD< f64, SD0, SD1, SD2, IntConst<int, FE_TONEAREST> > > } } },
	{ "fmsub.d/rne",    MASK_MULADD,  OPCODE_FMSUB(1, 0),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64MSUB< f64, SD0, SD1, SD2, IntConst<int, FE_TONEAREST> > > } } },
	{ "fmsub.d/rtz",    MASK_MULADD,  OPCODE_FMSUB(1, 1),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64MSUB< f64, SD0, SD1, SD2, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fmsub.d/rdn",    MASK_MULADD,  OPCODE_FMSUB(1, 2),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64MSUB< f64, SD0, SD1, SD2, IntConst<int, FE_DOWNWARD> > > } } },
	{ "fmsub.d/rup",    MASK_MULADD,  OPCODE_FMSUB(1, 3),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64MSUB< f64, SD0, SD1, SD2, IntConst<int, FE_UPWARD> > > } } },
	{ "fmsub.d/rmm",    MASK_MULADD,  OPCODE_FMSUB(1, 4),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64MSUB< f64, SD0, SD1, SD2, IntConst<int, FE_TONEAREST> > > } } },
	{ "fnmadd.d/rne",   MASK_MULADD,  OPCODE_FNMADD(1, 0),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64NMADD< f64, SD0, SD1, SD2, IntConst<int, FE_TONEAREST> > > } } },
	{ "fnmadd.d/rtz",   MASK_MULADD,  OPCODE_FNMADD(1, 1),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64NMADD< f64, SD0, SD1, SD2, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fnmadd.d/rdn",   MASK_MULADD,  OPCODE_FNMADD(1, 2),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64NMADD< f64, SD0, SD1, SD2, IntConst<int, FE_DOWNWARD> > > } } },
	{ "fnmadd.d/rup",   MASK_MULADD,  OPCODE_FNMADD(1, 3),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64NMADD< f64, SD0, SD1, SD2, IntConst<int, FE_UPWARD> > > } } },
	{ "fnmadd.d/rmm",   MASK_MULADD,  OPCODE_FNMADD(1, 4),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64NMADD< f64, SD0, SD1, SD2, IntConst<int, FE_TONEAREST> > > } } },
	{ "fnmsub.d/rne",   MASK_MULADD,  OPCODE_FNMSUB(1, 0),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64NMSUB< f64, SD0, SD1, SD2, IntConst<int, FE_TONEAREST> > > } } },
	{ "fnmsub.d/rtz",   MASK_MULADD,  OPCODE_FNMSUB(1, 1),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64NMSUB< f64, SD0, SD1, SD2, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fnmsub.d/rdn",   MASK_MULADD,  OPCODE_FNMSUB(1, 2),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64NMSUB< f64, SD0, SD1, SD2, IntConst<int, FE_DOWNWARD> > > } } },
	{ "fnmsub.d/rup",   MASK_MULADD,  OPCODE_FNMSUB(1, 3),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64NMSUB< f64, SD0, SD1, SD2, IntConst<int, FE_UPWARD> > > } } },
    { "fnmsub.d/rmm",   MASK_MULADD,  OPCODE_FNMSUB(1, 4),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV64NMSUB< f64, SD0, SD1, SD2, IntConst<int, FE_TONEAREST> > > } } },

	//conversion
	//{Name,            Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
	{ "fcvt.w.d/rne",   MASK_FCVT,    OPCODE_FCVT(0x61, 0, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetSext< D0, RISCV64FPToInt< s32, SD0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.w.d/rtz",   MASK_FCVT,    OPCODE_FCVT(0x61, 0, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetSext< D0, RISCV64FPToInt< s32, SD0, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fcvt.w.d/rdn",   MASK_FCVT,    OPCODE_FCVT(0x61, 0, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetSext< D0, RISCV64FPToInt< s32, SD0, IntConst<int, FE_DOWNWARD> > > } } },
    { "fcvt.w.d/rup",   MASK_FCVT,    OPCODE_FCVT(0x61, 0, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetSext< D0, RISCV64FPToInt< s32, SD0, IntConst<int, FE_UPWARD> > > } } },
	{ "fcvt.w.d/rmm",   MASK_FCVT,    OPCODE_FCVT(0x61, 0, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetSext< D0, RISCV64FPToInt< s32, SD0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.wu.d/rne",  MASK_FCVT,    OPCODE_FCVT(0x61, 1, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u32, SD0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.wu.d/rtz",  MASK_FCVT,    OPCODE_FCVT(0x61, 1, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u32, SD0, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fcvt.wu.d/rdn",  MASK_FCVT,    OPCODE_FCVT(0x61, 1, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u32, SD0, IntConst<int, FE_DOWNWARD> > > } } },
    { "fcvt.wu.d/rup",  MASK_FCVT,    OPCODE_FCVT(0x61, 1, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u32, SD0, IntConst<int, FE_UPWARD> > > } } },
	{ "fcvt.wu.d/rmm",  MASK_FCVT,    OPCODE_FCVT(0x61, 1, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u32, SD0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.d.w/rne",   MASK_FCVT,    OPCODE_FCVT(0x69, 0, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< s32, S0>, IntConst<int, FE_TONEAREST> > > } } }, //これでいいの？
	{ "fcvt.d.w/rtz",   MASK_FCVT,    OPCODE_FCVT(0x69, 0, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< s32, S0>, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fcvt.d.w/rdn",   MASK_FCVT,    OPCODE_FCVT(0x69, 0, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< s32, S0>, IntConst<int, FE_DOWNWARD> > > } } },
    { "fcvt.d.w/rup",   MASK_FCVT,    OPCODE_FCVT(0x69, 0, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< s32, S0>, IntConst<int, FE_UPWARD> > > } } },
	{ "fcvt.d.w/rmm",   MASK_FCVT,    OPCODE_FCVT(0x69, 0, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< s32, S0>, IntConst<int, FE_TONEAREST> > > } } },
    { "fcvt.d.wu/rne",  MASK_FCVT,    OPCODE_FCVT(0x69, 1, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< u32, S0>, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.d.wu/rtz",  MASK_FCVT,    OPCODE_FCVT(0x69, 1, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< u32, S0>, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fcvt.d.wu/rdn",  MASK_FCVT,    OPCODE_FCVT(0x69, 1, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< u32, S0>, IntConst<int, FE_DOWNWARD> > > } } },
    { "fcvt.d.wu/rup",  MASK_FCVT,    OPCODE_FCVT(0x69, 1, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< u32, S0>, IntConst<int, FE_UPWARD> > > } } },
	{ "fcvt.d.wu/rmm",  MASK_FCVT,    OPCODE_FCVT(0x69, 1, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< u32, S0>, IntConst<int, FE_TONEAREST> > > } } },

	//move
	//{Name,        Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
	{ "fsgnj.d",    MASK_FLOAT,   OPCODE_FLOAT(0x11, 0),    1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, FPDoubleCopySign<S1, S0> > } } },
	{ "fsgnjn.d",   MASK_FLOAT,   OPCODE_FLOAT(0x11, 1),    1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, FPDoubleCopySignNeg<S1, S0> > } } },
	{ "fsgnjx.d",   MASK_FLOAT,   OPCODE_FLOAT(0x11, 2),    1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, FPDoubleCopySignXor<S1, S0> > } } },


	//compare
	//{Name,        Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
	{ "feq.d",      MASK_FLOAT,   OPCODE_FLOAT(0x51, 2),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV64FPEqual< f64, SD0, SD1> > } } },
	{ "flt.d",      MASK_FLOAT,   OPCODE_FLOAT(0x51, 1),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV64FPLess< f64, SD0, SD1> > } } },
	{ "fle.d",      MASK_FLOAT,   OPCODE_FLOAT(0x51, 0),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV64FPLessEqual< f64, SD0, SD1> > } } },

	//class よくわからない

	//RV64D
	//conversion
	{ "fcvt.l.d/rne",   MASK_FCVT,    OPCODE_FCVT(0x61, 2, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToInt< s64, SD0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.l.d/rtz",   MASK_FCVT,    OPCODE_FCVT(0x61, 2, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToInt< s64, SD0, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fcvt.l.d/rdn",   MASK_FCVT,    OPCODE_FCVT(0x61, 2, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToInt< s64, SD0, IntConst<int, FE_DOWNWARD> > > } } },
	{ "fcvt.l.d/rup",   MASK_FCVT,    OPCODE_FCVT(0x61, 2, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToInt< s64, SD0, IntConst<int, FE_UPWARD> > > } } },
	{ "fcvt.l.d/rmm",   MASK_FCVT,    OPCODE_FCVT(0x61, 2, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToInt< s64, SD0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.lu.d/rne",  MASK_FCVT,    OPCODE_FCVT(0x61, 3, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u64, SD0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.lu.d/rtz",  MASK_FCVT,    OPCODE_FCVT(0x61, 3, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u64, SD0, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fcvt.lu.d/rdn",  MASK_FCVT,    OPCODE_FCVT(0x61, 3, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u64, SD0, IntConst<int, FE_DOWNWARD> > > } } },
	{ "fcvt.lu.d/rup",  MASK_FCVT,    OPCODE_FCVT(0x61, 3, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u64, SD0, IntConst<int, FE_UPWARD> > > } } },
	{ "fcvt.lu.d/rmm",  MASK_FCVT,    OPCODE_FCVT(0x61, 3, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FPToIntU< u64, SD0, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.d.l/rne",   MASK_FCVT,    OPCODE_FCVT(0x69, 2, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< s64, S0>, IntConst<int, FE_TONEAREST> > > } } }, //これでいいの？
	{ "fcvt.d.l/rtz",   MASK_FCVT,    OPCODE_FCVT(0x69, 2, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< s64, S0>, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fcvt.d.l/rdn",   MASK_FCVT,    OPCODE_FCVT(0x69, 2, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< s64, S0>, IntConst<int, FE_DOWNWARD> > > } } },
	{ "fcvt.d.l/rup",   MASK_FCVT,    OPCODE_FCVT(0x69, 2, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< s64, S0>, IntConst<int, FE_UPWARD> > > } } },
	{ "fcvt.d.l/rmm",   MASK_FCVT,    OPCODE_FCVT(0x69, 2, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< s64, S0>, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.d.lu/rne",  MASK_FCVT,    OPCODE_FCVT(0x69, 3, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< u64, S0>, IntConst<int, FE_TONEAREST> > > } } },
	{ "fcvt.d.lu/rtz",  MASK_FCVT,    OPCODE_FCVT(0x69, 3, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< u64, S0>, IntConst<int, FE_TOWARDZERO> > > } } },
	{ "fcvt.d.lu/rdn",  MASK_FCVT,    OPCODE_FCVT(0x69, 3, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< u64, S0>, IntConst<int, FE_DOWNWARD> > > } } },
	{ "fcvt.d.lu/rup",  MASK_FCVT,    OPCODE_FCVT(0x69, 3, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< u64, S0>, IntConst<int, FE_UPWARD> > > } } },
	{ "fcvt.d.lu/rmm",  MASK_FCVT,    OPCODE_FCVT(0x69, 3, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< u64, S0>, IntConst<int, FE_TONEAREST> > > } } },

	//move
	//{Name,        Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
	{ "fmv.x.d",    MASK_MV,      OPCODE_MV(0x71),          1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, -1, -1, -1},   Set< D0, S0> } } }, //　IEEE 754-2008?
	{ "fmv.d.x",    MASK_MV,      OPCODE_MV(0x79),          1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, -1, -1, -1},   Set< D0, S0> } } }, //これでいいのか？

  //class
	//{Name,        Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
	{ "fclass.d",   MASK_FCLASS,  OPCODE_FCLASS(0x71),      1,{ { OpClassCode::fADD,   {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV64FCLASS< f64, SD0> > } } },



};

//
// RISCV64Converter
//


RISCV64Converter::RISCV64Converter()
{
    AddToOpMap(m_OpDefsBase, sizeof(m_OpDefsBase)/sizeof(OpDef));
}

RISCV64Converter::~RISCV64Converter()
{
}

// srcTemplate に対応するオペランドの種類と，レジスタならば番号を，即値ならばindexを返す
std::pair<RISCV64Converter::OperandType, int> RISCV64Converter::GetActualSrcOperand(int srcTemplate, const DecodedInsn& decoded) const
{
    typedef std::pair<OperandType, int> RetType;
    if (srcTemplate == -1) {
        return RetType(OpInfoType::NONE, 0);
    }
    else if (ImmTemplateBegin <= srcTemplate && srcTemplate <= ImmTemplateEnd) {
        return RetType(OpInfoType::IMM, srcTemplate - ImmTemplateBegin);
    }
    else  {
        return RetType(OpInfoType::REG, GetActualRegNumber(srcTemplate, decoded) );
    }
}

// regTemplate から実際のレジスタ番号を取得する
int RISCV64Converter::GetActualRegNumber(int regTemplate, const DecodedInsn& decoded) const
{
    if (regTemplate == -1) {
        return -1;
    }
    else if (RegTemplateBegin <= regTemplate && regTemplate <= RegTemplateEnd) {
        return decoded.Reg[regTemplate - RegTemplateBegin];
    }
    else if (0 <= regTemplate && regTemplate < RISCV64Info::RegisterCount) {
        return regTemplate;
    }
    else {
        ASSERT(0, "RISCV64Converter Logic Error : invalid regTemplate");
        return -1;
    }
}

// レジスタ番号regNumがゼロレジスタかどうかを判定する
bool RISCV64Converter::IsZeroReg(int regNum) const
{
    const int IntZeroReg = 0;
    const int FPZeroReg = 32;

    return regNum == IntZeroReg || regNum == FPZeroReg;
}


void RISCV64Converter::RISCVUnknownOperation(OpEmulationState* opState)
{
    u32 codeWord = static_cast<u32>( SrcOperand<0>()(opState) );

    DecoderType decoder;
    DecodedInsn decoded;
    decoder.Decode( codeWord, &decoded);

    stringstream ss;
    u32 opcode = codeWord & 0x7f;
    ss << "unknown instruction : " << hex << setw(8) << codeWord << endl;
    ss << "\topcode : " << hex << opcode << endl;
    ss << "\timm[1] : " << hex << decoded.Imm[1] << endl;

    THROW_RUNTIME_ERROR(ss.str().c_str());
}

const RISCV64Converter::OpDef* RISCV64Converter::GetOpDefUnknown() const
{
    return &m_OpDefUnknown;
}
