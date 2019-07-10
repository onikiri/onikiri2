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

#include "Emu/RISCV32Linux/RISCV32Converter.h"

//#include "Emu/Utility/GenericOperation.h"
#include "Emu/Utility/OpEmulationState.h"

#include "Emu/RISCV32Linux/RISCV32Info.h"
#include "Emu/RISCV32Linux/RISCV32OpInfo.h"
#include "Emu/RISCV32Linux/RISCV32Decoder.h"
#include "Emu/RISCV32Linux/RISCV32Operation.h"

using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::RISCV32Linux;
using namespace Onikiri::EmulatorUtility::Operation;
using namespace Onikiri::RISCV32Linux::Operation;

//
// 命令の定義
//

namespace {

    // 各命令形式に対するオペコードを得るためのマスク (0のビットが引数)
    static const u32 MASK_EXACT = 0xffffffff;  // 全bitが一致

    static const u32 MASK_LUI =     0x0000007f; // U-type, opcode
    static const u32 MASK_AUIPC =   0x0000007f; // U-type, opcode

    static const u32 MASK_IMM =     0x0000707f; // I-type, funct3 + opcode
    static const u32 MASK_SHIFT =   0xfe00707f; // I-type?, funct3 + opcode
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

    static const u32 MASK_ATOMIC =  0xf800707f; // R-type, funct5 + funct3 + opcode

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
#define OPCODE_SHIFT(f7, f3) (u32)(((f7) << 25) | ((f3) << 12) | 0x13)
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


#define OPCODE_ATOMIC(width, funct5) (u32)(((funct5) << 27) | ((width) << 12) | 0x2f)

#define OPCODE_FLD(f)  (u32)(((f) << 12) | 0x07)
#define OPCODE_FST(f)  (u32)(((f) << 12) | 0x27)

#define OPCODE_FLOAT(f7, f3) (u32)(((f7) << 25) | ((f3) << 12) | 0x53)
#define OPCODE_SQRT(f7, f3) (u32)(((f7) << 25) | (0 << 20) | ((f3) << 12) | 0x53)
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

}

#define RISCV32_DSTOP(n) RISCV32DstOperand<n>
#define RISCV32_SRCOP(n) RISCV32SrcOperand<n>
#define RISCV32_SRCOPFLOAT(n) AsFP< f32, Cast< u32, SrcOperand<n> > > // lower 32-bit holds float value
#define RISCV32_SRCOPDOUBLE(n) AsFP< f64, SrcOperand<n> >

#define D0 RISCV32_DSTOP(0)
#define D1 RISCV32_DSTOP(1)
#define S0 RISCV32_SRCOP(0)
#define S1 RISCV32_SRCOP(1)
#define S2 RISCV32_SRCOP(2)
#define S3 RISCV32_SRCOP(3)
#define SF0 RISCV32_SRCOPFLOAT(0)
#define SF1 RISCV32_SRCOPFLOAT(1)
#define SF2 RISCV32_SRCOPFLOAT(2)
#define SF3 RISCV32_SRCOPFLOAT(3)
#define SD0 RISCV32_SRCOPDOUBLE(0)
#define SD1 RISCV32_SRCOPDOUBLE(1)
#define SD2 RISCV32_SRCOPDOUBLE(2)
#define SD3 RISCV32_SRCOPDOUBLE(3)

// 投機的にフェッチされたときにはエラーにせず，実行されたときにエラーにする
// syscallにすることにより，直前までの命令が完了してから実行される (実行は投機的でない)
RISCV32Converter::OpDef RISCV32Converter::m_OpDefUnknown = 
    {"unknown", MASK_EXACT, 0,  1, {{OpClassCode::UNDEF,    {-1, -1}, {I0, -1, -1, -1}, RISCV32Converter::RISCVUnknownOperation}}};


// branchは，OpInfo 列の最後じゃないとだめ
RISCV32Converter::OpDef RISCV32Converter::m_OpDefsBase[] = 
{
    // LUI/AUIPC
    //{Name,    Mask,       Opcode,         nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"lui",     MASK_LUI,   OPCODE_LUI(),   1,  { {OpClassCode::iALU,   {R0, -1},   {I0, -1, -1, -1},   Set<D0, RISCV32Lui<S0> >} } },
    {"auipc",   MASK_AUIPC, OPCODE_AUIPC(), 1,  { {OpClassCode::iALU,   {R0, -1},   {I0, -1, -1, -1},   Set<D0, RISCV32Auipc<S0> >} } },
    
    // IMM
    //{Name,    Mask,       Opcode,         nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"addi",    MASK_IMM,   OPCODE_IMM(0),  1,  { {OpClassCode::iALU,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, IntAdd<u32, S0, S1> > } } },
    {"slti",    MASK_IMM,   OPCODE_IMM(2),  1,  { {OpClassCode::iALU,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, RISCV32Compare<S0, S1, IntCondLessSigned<u32> > > } } },
    {"sltiu",   MASK_IMM,   OPCODE_IMM(3),  1,  { {OpClassCode::iALU,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, RISCV32Compare<S0, S1, IntCondLessUnsigned<u32> > > } } },
    {"xori",    MASK_IMM,   OPCODE_IMM(4),  1,  { {OpClassCode::iALU,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, BitXor<u32, S0, S1> > } } },
    {"ori",     MASK_IMM,   OPCODE_IMM(6),  1,  { {OpClassCode::iALU,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, BitOr<u32, S0, S1> > } } },
    {"andi",    MASK_IMM,   OPCODE_IMM(7),  1,  { {OpClassCode::iALU,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, BitAnd<u32, S0, S1> > } } },
    
    // SHIFT
    //{Name,    Mask,       Opcode,                 nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"slli",    MASK_SHIFT, OPCODE_SHIFT(0x00, 1),  1,  { {OpClassCode::iSFT,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, LShiftL<u32, S0, S1, 0x1f > > } } },
    {"srli",    MASK_SHIFT, OPCODE_SHIFT(0x00, 5),  1,  { {OpClassCode::iSFT,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, LShiftR<u32, S0, S1, 0x1f > > } } },
    {"srai",    MASK_SHIFT, OPCODE_SHIFT(0x20, 5),  1,  { {OpClassCode::iSFT,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, AShiftR<u32, S0, S1, 0x1f > > } } },

    // INT
    //{Name,    Mask,       Opcode,                 nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"add",     MASK_INT,   OPCODE_INT(0x00, 0),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, IntAdd<u32, S0, S1> > } } },
    {"sub",     MASK_INT,   OPCODE_INT(0x20, 0),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, IntSub<u32, S0, S1> > } } },
    {"sll",     MASK_INT,   OPCODE_INT(0x00, 1),    1,  { {OpClassCode::iSFT,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, LShiftL<u32, S0, S1, 0x1f > > } } },
    {"slt",     MASK_INT,   OPCODE_INT(0x00, 2),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, RISCV32Compare<S0, S1, IntCondLessSigned<u32> > > } } },
    {"sltu",    MASK_INT,   OPCODE_INT(0x00, 3),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, RISCV32Compare<S0, S1, IntCondLessUnsigned<u32> > > } } },
    {"xor",     MASK_INT,   OPCODE_INT(0x00, 4),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, BitXor<u32, S0, S1> > } } },
    {"srl",     MASK_INT,   OPCODE_INT(0x00, 5),    1,  { {OpClassCode::iSFT,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, LShiftR<u32, S0, S1, 0x1f > > } } },
    {"sra",     MASK_INT,   OPCODE_INT(0x20, 5),    1,  { {OpClassCode::iSFT,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, AShiftR<u32, S0, S1, 0x1f > > } } },
    {"or",      MASK_INT,   OPCODE_INT(0x00, 6),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, BitOr<u32, S0, S1> > } } },
    {"and",     MASK_INT,   OPCODE_INT(0x00, 7),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, BitAnd<u32, S0, S1> > } } },

    // JAL
    // J must be placed before JAL, because MASK_JAL/OPCODE_JAL include J
    //{Name,    Mask,       Opcode,         nOp,{ OpClassCode,              Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"j",       MASK_J,     OPCODE_J(),     1,  { { OpClassCode::iJUMP,     {-1, -1},   {I0, -1, -1, -1},   RISCV32BranchRelUncond<S0> } } },
    {"jal",     MASK_JAL,   OPCODE_JAL(),   1,  { { OpClassCode::CALL_JUMP, {R0, -1},   {I0, -1, -1, -1},   RISCV32CallRelUncond<D0, S0> } } },
    
    // JALR
    // CALL/RET must be placed before JALR, because MASK_JALR/OPCODE_JALR include CALL/RET
    //    rd    rs1    rs1=rd   RAS action
    //    !link !link  -        none
    //    !link link   -        pop
    //    link  !link  -        push
    //    link  link   0        push and pop (not implemented; the action is 'none' for now)
    //    link  link   1        push
    //{Name,    Mask,           Opcode,                 nOp,{ OpClassCode,              Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"jalr",    MASK_CALLRET,   OPCODE_CALLRET(1, 5),   1,  { { OpClassCode::iJUMP,     {R0, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_CALLRET,   OPCODE_CALLRET(5, 1),   1,  { { OpClassCode::iJUMP,     {R0, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_CALLRET,   OPCODE_CALLRET(1, 1),   1,  { { OpClassCode::CALL_JUMP, {R0, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_CALLRET,   OPCODE_CALLRET(5, 5),   1,  { { OpClassCode::CALL_JUMP, {R0, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_CALL,      OPCODE_CALL(5),         1,  { { OpClassCode::CALL_JUMP, {R0, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_CALL,      OPCODE_CALL(1),         1,  { { OpClassCode::CALL_JUMP, {R0, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_RET,       OPCODE_RET(5),          1,  { { OpClassCode::RET,       {R0, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_RET,       OPCODE_RET(1),          1,  { { OpClassCode::RET,       {R0, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_JALR,      OPCODE_JALR(),          1,  { { OpClassCode::iJUMP,     {R0, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },

    // Branch
    //{Name,    Mask,       Opcode,         nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"beq",     MASK_BR,    OPCODE_BR(0),   1,  { { OpClassCode::iBC,   {-1, -1},   {R0, R1, I0, -1},   RISCV32BranchRelCond<S2, Compare<S0, S1, IntCondEqual<u32> > > } } },
    {"bne",     MASK_BR,    OPCODE_BR(1),   1,  { { OpClassCode::iBC,   {-1, -1},   {R0, R1, I0, -1},   RISCV32BranchRelCond<S2, Compare<S0, S1, IntCondNotEqual<u32> > > } } },
    {"blt",     MASK_BR,    OPCODE_BR(4),   1,  { { OpClassCode::iBC,   {-1, -1},   {R0, R1, I0, -1},   RISCV32BranchRelCond<S2, Compare<S0, S1, IntCondLessSigned<u32> > > } } },
    {"bge",     MASK_BR,    OPCODE_BR(5),   1,  { { OpClassCode::iBC,   {-1, -1},   {R0, R1, I0, -1},   RISCV32BranchRelCond<S2, Compare<S0, S1, IntCondGreaterEqualSigned<u32> > > } } },
    {"bltu",    MASK_BR,    OPCODE_BR(6),   1,  { { OpClassCode::iBC,   {-1, -1},   {R0, R1, I0, -1},   RISCV32BranchRelCond<S2, Compare<S0, S1, IntCondLessUnsigned<u32> > > } } },
    {"bgeu",    MASK_BR,    OPCODE_BR(7),   1,  { { OpClassCode::iBC,   {-1, -1},   {R0, R1, I0, -1},   RISCV32BranchRelCond<S2, Compare<S0, S1, IntCondGreaterEqualUnsigned<u32> > > } } },

    // Store
    //{Name,    Mask,       Opcode,         nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"sb",      MASK_ST,    OPCODE_ST(0),   1,  { { OpClassCode::iST,   {-1, -1},   {R1, R0, I0, -1},   Store<u8, S0, RISCV32Addr<S1, S2> > } } },
    {"sh",      MASK_ST,    OPCODE_ST(1),   1,  { { OpClassCode::iST,   {-1, -1},   {R1, R0, I0, -1},   Store<u16, S0, RISCV32Addr<S1, S2> > } } },
    {"sw",      MASK_ST,    OPCODE_ST(2),   1,  { { OpClassCode::iST,   {-1, -1},   {R1, R0, I0, -1},   Store<u32, S0, RISCV32Addr<S1, S2> > } } },

    // Load
    //{Name,    Mask,       Opcode,         nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"lb",      MASK_LD,    OPCODE_LD(0),   1,  { { OpClassCode::iLD,   {R0, -1},   {R1, I0, -1, -1},   SetSext<D0, Load<u8,  RISCV32Addr<S0, S1> > > } } },
    {"lh",      MASK_LD,    OPCODE_LD(1),   1,  { { OpClassCode::iLD,   {R0, -1},   {R1, I0, -1, -1},   SetSext<D0, Load<u16, RISCV32Addr<S0, S1> > > } } },
    {"lw",      MASK_LD,    OPCODE_LD(2),   1,  { { OpClassCode::iLD,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, Load<u32, RISCV32Addr<S0, S1> > > } } },
    {"lb",      MASK_LD,    OPCODE_LD(4),   1,  { { OpClassCode::iLD,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, Load<u8,  RISCV32Addr<S0, S1> > > } } },
    {"lh",      MASK_LD,    OPCODE_LD(5),   1,  { { OpClassCode::iLD,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, Load<u16, RISCV32Addr<S0, S1> > > } } },

    // System
    //{Name,    Mask,       Opcode,         nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"ecall",   MASK_EXACT, OPCODE_ECALL(),   2,  {
        {OpClassCode::syscall,          {17, -1}, {17, 10, 11, -1}, RISCV32SyscallSetArg} ,
        {OpClassCode::syscall_branch,   {10, -1}, {17, 12, 13, -1}, RISCV32SyscallCore},
    }},

    // Fence Instructions
    // It is not always necessary to stop all the instructions
    // but for the sake of simplicity, I implemented by using OpClalssCode::syscall
    {"fence"    , 0xf00fffff, 0x0000000f,   1,  { { OpClassCode::syscall,   {-1, -1},   {-1, -1, -1, -1},   NoOperation } } },
    {"fence.tso", MASK_EXACT, 0x8330000f,   1,  { { OpClassCode::syscall,   {-1, -1},   {-1, -1, -1, -1},   NoOperation } } },
    {"fence.i"  , MASK_EXACT, 0x0000100f,   1,  { { OpClassCode::syscall,   {-1, -1},   {-1, -1, -1, -1},   NoOperation } } },


    // Csr
    {"csrrw",   MASK_CSR,   OPCODE_CSR(1),   2,  {
        {OpClassCode::syscall,          {-1, -1}, {-1, -1, -1, -1}, RISCV32SyscallSetArg} ,
        {OpClassCode::syscall_branch,   {R0, -1}, {R1, I0, -1, -1}, RISCV32CSRRW<D0, S0, S1>},
    }},
    {"csrrs",   MASK_CSR,   OPCODE_CSR(2),   2,  {
        {OpClassCode::syscall,          {-1, -1}, {-1, -1, -1, -1}, RISCV32SyscallSetArg} ,
        {OpClassCode::syscall_branch,   {R0, -1}, {R1, I0, -1, -1}, RISCV32CSRRS<D0, S0, S1>},
    }},
    {"csrrc",   MASK_CSR,   OPCODE_CSR(3),   2,  {
        {OpClassCode::syscall,          {-1, -1}, {-1, -1, -1, -1}, RISCV32SyscallSetArg} ,
        {OpClassCode::syscall_branch,   {R0, -1}, {R1, I0, -1, -1}, RISCV32CSRRC<D0, S0, S1>},
    }},
    {"csrrwi",   MASK_CSR,   OPCODE_CSR(5),   2,  {
        {OpClassCode::syscall,          {-1, -1}, {-1, -1, -1, -1}, RISCV32SyscallSetArg} ,
        {OpClassCode::syscall_branch,   {R0, -1}, {I0, I1, -1, -1}, RISCV32CSRRW<D0, S0, S1>},
    }},
    {"csrrsi",   MASK_CSR,   OPCODE_CSR(6),   2,  {
        {OpClassCode::syscall,          {-1, -1}, {-1, -1, -1, -1}, RISCV32SyscallSetArg} ,
        {OpClassCode::syscall_branch,   {R0, -1}, {I0, I1, -1, -1}, RISCV32CSRRS<D0, S0, S1>},
    }},
    {"csrrci",   MASK_CSR,   OPCODE_CSR(7),   2,  {
        {OpClassCode::syscall,          {-1, -1}, {-1, -1, -1, -1}, RISCV32SyscallSetArg} ,
        {OpClassCode::syscall_branch,   {R0, -1}, {I0, I1, -1, -1}, RISCV32CSRRC<D0, S0, S1>},
    }},

    
    // RV32M
    //{Name,    Mask,       Opcode,                 nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"mul",     MASK_INT,   OPCODE_INT(0x01, 0),    1,  { {OpClassCode::iMUL,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, IntMul<u32, S0, S1> > } } },
    {"mulh",    MASK_INT,   OPCODE_INT(0x01, 1),    1,  { {OpClassCode::iMUL,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, IntUMulh32<s64, s64, Cast<s32, S0>, Cast<s32, S1> > > } } },
    {"mulhsu",  MASK_INT,   OPCODE_INT(0x01, 2),    1,  { {OpClassCode::iMUL,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, IntUMulh32<s64, u64, Cast<s32, S0>, Cast<u32, S1> > > } } },
    {"mulhu",   MASK_INT,   OPCODE_INT(0x01, 3),    1,  { {OpClassCode::iMUL,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, IntUMulh32<u64, u64, Cast<u32, S0>, Cast<u32, S1> > > } } },
    {"div",     MASK_INT,   OPCODE_INT(0x01, 4),    1,  { {OpClassCode::iDIV,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, RISCV32IntDiv<S0, S1> > } } },
    {"divu",    MASK_INT,   OPCODE_INT(0x01, 5),    1,  { {OpClassCode::iDIV,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, RISCV32IntDivu<S0, S1> > } } },
    {"rem",     MASK_INT,   OPCODE_INT(0x01, 6),    1,  { {OpClassCode::iDIV,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, RISCV32IntRem<S0, S1> > } } },
    {"remu",    MASK_INT,   OPCODE_INT(0x01, 7),    1,  { {OpClassCode::iDIV,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, RISCV32IntRemu<S0, S1> > } } },

    // RV32A
    // Only for single thread execution
    // It is not always necessary to stop all the instructions
    // but for the sake of simplicity, I implemented by using OpClalssCode::syscall
    //{Name,       Mask,        Opcode,               nOp, { OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    { "lr.w"     , MASK_ATOMIC, OPCODE_ATOMIC(2, 2),  1, { { OpClassCode::syscall, { R0, -1 }, { R1, -1, -1, -1 }, RISCV32LoadReserved<u32, D0, S0> } } },
    { "sc.w"     , MASK_ATOMIC, OPCODE_ATOMIC(2, 3),  1, { { OpClassCode::syscall, { R0, -1 }, { R1, R2, -1, -1 }, RISCV32StoreConditional<u32, D0, S1, S0> } } },
    { "amoswap.w", MASK_ATOMIC, OPCODE_ATOMIC(2, 1),  1, { { OpClassCode::syscall, { R0, -1 }, { R1, R2, -1, -1 }, Sequence2<SetSext<D0, RISCV32AtomicLoad<u32, S0> >, RISCV32AtomicStore<u32, S1, S0> > } } },
    { "amoadd.w" , MASK_ATOMIC, OPCODE_ATOMIC(2, 0),  1, { { OpClassCode::syscall, { R0, -1 }, { R1, R2, -1, -1 }, RISCV32AtomicStore<u32, IntAdd    <u32, S1, TeeSetSext<D0, RISCV32AtomicLoad<u32, S0> > >, S0> } } },
    { "amoxor.w" , MASK_ATOMIC, OPCODE_ATOMIC(2, 4),  1, { { OpClassCode::syscall, { R0, -1 }, { R1, R2, -1, -1 }, RISCV32AtomicStore<u32, BitXor    <u32, S1, TeeSetSext<D0, RISCV32AtomicLoad<u32, S0> > >, S0> } } },
    { "amoand.w" , MASK_ATOMIC, OPCODE_ATOMIC(2, 12), 1, { { OpClassCode::syscall, { R0, -1 }, { R1, R2, -1, -1 }, RISCV32AtomicStore<u32, BitAnd    <u32, S1, TeeSetSext<D0, RISCV32AtomicLoad<u32, S0> > >, S0> } } },
    { "amoor.w"  , MASK_ATOMIC, OPCODE_ATOMIC(2, 8),  1, { { OpClassCode::syscall, { R0, -1 }, { R1, R2, -1, -1 }, RISCV32AtomicStore<u32, BitOr     <u32, S1, TeeSetSext<D0, RISCV32AtomicLoad<u32, S0> > >, S0> } } },
    { "amomin.w" , MASK_ATOMIC, OPCODE_ATOMIC(2, 16), 1, { { OpClassCode::syscall, { R0, -1 }, { R1, R2, -1, -1 }, RISCV32AtomicStore<u32, RISCV32MIN<s32, S1, TeeSetSext<D0, RISCV32AtomicLoad<u32, S0> > >, S0> } } },
    { "amomax.w" , MASK_ATOMIC, OPCODE_ATOMIC(2, 20), 1, { { OpClassCode::syscall, { R0, -1 }, { R1, R2, -1, -1 }, RISCV32AtomicStore<u32, RISCV32MAX<s32, S1, TeeSetSext<D0, RISCV32AtomicLoad<u32, S0> > >, S0> } } },
    { "amominu.w", MASK_ATOMIC, OPCODE_ATOMIC(2, 24), 1, { { OpClassCode::syscall, { R0, -1 }, { R1, R2, -1, -1 }, RISCV32AtomicStore<u32, RISCV32MIN<u32, S1, TeeSetSext<D0, RISCV32AtomicLoad<u32, S0> > >, S0> } } },
    { "amomaxu.w", MASK_ATOMIC, OPCODE_ATOMIC(2, 28), 1, { { OpClassCode::syscall, { R0, -1 }, { R1, R2, -1, -1 }, RISCV32AtomicStore<u32, RISCV32MAX<u32, S1, TeeSetSext<D0, RISCV32AtomicLoad<u32, S0> > >, S0> } } },

    // RV32F
    // LOAD/STORE
    //{Name,    Mask,       Opcode,           nOp,{ OpClassCode,        Dst[],      Src[],               OpInfoType::EmulationFunc}[]}
    { "flw",    MASK_LD,    OPCODE_FLD(2),    1,{ { OpClassCode::fLD,   {R0, -1},   {R1, I0, -1, -1},    Set<D0, RISCV32NanBoxing< AsFP<f32, Load<u32, RISCV32Addr<S0, S1> > > > > } } },
    { "fsw",    MASK_ST,    OPCODE_FST(2),    1,{ { OpClassCode::fST,   {-1, -1},   {R1, R0, I0, -1},    Store<u32, S0, RISCV32Addr<S1, S2> > } } },

    // FLOAT
    //  round mode (bit 14-12)
    //  000: RNE Round to Nearest, ties to Even (直近に丸める、真ん中なら偶数に丸める)
    //  001: RTZ Round towards Zero
    //  010: RDN Round Down (towards −infinity)
    //  011: RUP Round Up (towards +infinity)
    //  100: RMM Round to Nearest, ties to Max Magnitude (直近に丸める、真ん中なら絶対値が大きい方に丸める)
    //  111: Use a round mode in FCSR
    //{Name,            Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    { "fadd.s/rne",     MASK_FLOAT,   OPCODE_FLOAT(0x00, 0),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPAdd< f32, SF0, SF1, IntConst<int, FE_TONEAREST> > > > } } },
    { "fadd.s/rtz",     MASK_FLOAT,   OPCODE_FLOAT(0x00, 1),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPAdd< f32, SF0, SF1, IntConst<int, FE_TOWARDZERO> > > > } } },
    { "fadd.s/rdn",     MASK_FLOAT,   OPCODE_FLOAT(0x00, 2),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPAdd< f32, SF0, SF1, IntConst<int, FE_DOWNWARD> > > > } } },
    { "fadd.s/rup",     MASK_FLOAT,   OPCODE_FLOAT(0x00, 3),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPAdd< f32, SF0, SF1, IntConst<int, FE_UPWARD> > > > } } },
    { "fadd.s/rmm",     MASK_FLOAT,   OPCODE_FLOAT(0x00, 4),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPAdd< f32, SF0, SF1, IntConst<int, FE_TONEAREST> > > > } } },
    { "fadd.s",         MASK_FLOAT,   OPCODE_FLOAT(0x00, 7),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPAdd< f32, SF0, SF1, RISCV32RoundModeFromFCSR > > > } } },

    { "fsub.s/rne",     MASK_FLOAT,   OPCODE_FLOAT(0x04, 0),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPSub< f32, SF0, SF1, IntConst<int, FE_TONEAREST> > > > } } },
    { "fsub.s/rtz",     MASK_FLOAT,   OPCODE_FLOAT(0x04, 1),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPSub< f32, SF0, SF1, IntConst<int, FE_TOWARDZERO> > > > } } },
    { "fsub.s/rdn",     MASK_FLOAT,   OPCODE_FLOAT(0x04, 2),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPSub< f32, SF0, SF1, IntConst<int, FE_DOWNWARD> > > > } } },
    { "fsub.s/rup",     MASK_FLOAT,   OPCODE_FLOAT(0x04, 3),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPSub< f32, SF0, SF1, IntConst<int, FE_UPWARD> > > > } } },
    { "fsub.s/rmm",     MASK_FLOAT,   OPCODE_FLOAT(0x04, 4),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPSub< f32, SF0, SF1, IntConst<int, FE_TONEAREST> > > > } } },
    { "fsub.s",         MASK_FLOAT,   OPCODE_FLOAT(0x04, 7),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPSub< f32, SF0, SF1, RISCV32RoundModeFromFCSR > > > } } },

    { "fmul.s/rne",     MASK_FLOAT,   OPCODE_FLOAT(0x08, 0),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPMul< f32, SF0, SF1, IntConst<int, FE_TONEAREST> > > > } } },
    { "fmul.s/rtz",     MASK_FLOAT,   OPCODE_FLOAT(0x08, 1),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPMul< f32, SF0, SF1, IntConst<int, FE_TOWARDZERO> > > > } } },
    { "fmul.s/rdn",     MASK_FLOAT,   OPCODE_FLOAT(0x08, 2),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPMul< f32, SF0, SF1, IntConst<int, FE_DOWNWARD> > > > } } },
    { "fmul.s/rup",     MASK_FLOAT,   OPCODE_FLOAT(0x08, 3),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPMul< f32, SF0, SF1, IntConst<int, FE_UPWARD> > > > } } },
    { "fmul.s/rmm",     MASK_FLOAT,   OPCODE_FLOAT(0x08, 4),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPMul< f32, SF0, SF1, IntConst<int, FE_TONEAREST> > > > } } },
    { "fmul.s",         MASK_FLOAT,   OPCODE_FLOAT(0x08, 7),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPMul< f32, SF0, SF1, RISCV32RoundModeFromFCSR > > > } } },

    { "fdiv.s/rne",     MASK_FLOAT,   OPCODE_FLOAT(0x0c, 0),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPDiv< f32, SF0, SF1, IntConst<int, FE_TONEAREST> > > > } } },
    { "fdiv.s/rtz",     MASK_FLOAT,   OPCODE_FLOAT(0x0c, 1),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPDiv< f32, SF0, SF1, IntConst<int, FE_TOWARDZERO> > > > } } },
    { "fdiv.s/rdn",     MASK_FLOAT,   OPCODE_FLOAT(0x0c, 2),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPDiv< f32, SF0, SF1, IntConst<int, FE_DOWNWARD> > > > } } },
    { "fdiv.s/rup",     MASK_FLOAT,   OPCODE_FLOAT(0x0c, 3),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPDiv< f32, SF0, SF1, IntConst<int, FE_UPWARD> > > > } } },
    { "fdiv.s/rmm",     MASK_FLOAT,   OPCODE_FLOAT(0x0c, 4),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPDiv< f32, SF0, SF1, IntConst<int, FE_TONEAREST> > > > } } },
    { "fdiv.s",         MASK_FLOAT,   OPCODE_FLOAT(0x0c, 7),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< FPDiv< f32, SF0, SF1, RISCV32RoundModeFromFCSR > > > } } },

    { "fsqrt.s/rne",    MASK_SQRT,    OPCODE_SQRT(0x2c, 0),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< FPSqrt< f32, SF0, IntConst<int, FE_TONEAREST> > > > } } },
    { "fsqrt.s/rtz",    MASK_SQRT,    OPCODE_SQRT(0x2c, 1),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< FPSqrt< f32, SF0, IntConst<int, FE_TOWARDZERO> > > > } } },
    { "fsqrt.s/rdn",    MASK_SQRT,    OPCODE_SQRT(0x2c, 2),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< FPSqrt< f32, SF0, IntConst<int, FE_DOWNWARD> > > > } } },
    { "fsqrt.s/rup",    MASK_SQRT,    OPCODE_SQRT(0x2c, 3),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< FPSqrt< f32, SF0, IntConst<int, FE_UPWARD> > > > } } },
    { "fsqrt.s/rmm",    MASK_SQRT,    OPCODE_SQRT(0x2c, 4),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< FPSqrt< f32, SF0, IntConst<int, FE_TONEAREST> > > > } } },
    { "fsqrt.s",        MASK_SQRT,    OPCODE_SQRT(0x2c, 7),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< FPSqrt< f32, SF0, RISCV32RoundModeFromFCSR > > > } } },

    { "fmin.s",         MASK_FLOAT,   OPCODE_FLOAT(0x14, 0),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< RISCV32FPMIN< f32, SF0, SF1> > > } } },
    { "fmax.s",         MASK_FLOAT,   OPCODE_FLOAT(0x14, 1),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< RISCV32FPMAX< f32, SF0, SF1> > > } } },

    { "fmadd.s/rne",    MASK_MULADD,  OPCODE_FMADD(0, 0),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32MADD< f32, SF0, SF1, SF2, IntConst<int, FE_TONEAREST> > > > } } },
    { "fmadd.s/rtz",    MASK_MULADD,  OPCODE_FMADD(0, 1),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32MADD< f32, SF0, SF1, SF2, IntConst<int, FE_TOWARDZERO> > > > } } },
    { "fmadd.s/rdn",    MASK_MULADD,  OPCODE_FMADD(0, 2),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32MADD< f32, SF0, SF1, SF2, IntConst<int, FE_DOWNWARD> > > > } } },
    { "fmadd.s/rup",    MASK_MULADD,  OPCODE_FMADD(0, 3),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32MADD< f32, SF0, SF1, SF2, IntConst<int, FE_UPWARD> > > > } } },
    { "fmadd.s/rmm",    MASK_MULADD,  OPCODE_FMADD(0, 4),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32MADD< f32, SF0, SF1, SF2, IntConst<int, FE_TONEAREST> > > > } } },
    { "fmadd.s",        MASK_MULADD,  OPCODE_FMADD(0, 7),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32MADD< f32, SF0, SF1, SF2, RISCV32RoundModeFromFCSR > > > } } },

    { "fmsub.s/rne",    MASK_MULADD,  OPCODE_FMSUB(0, 0),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32MSUB< f32, SF0, SF1, SF2, IntConst<int, FE_TONEAREST> > > > } } },
    { "fmsub.s/rtz",    MASK_MULADD,  OPCODE_FMSUB(0, 1),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32MSUB< f32, SF0, SF1, SF2, IntConst<int, FE_TOWARDZERO> > > > } } },
    { "fmsub.s/rdn",    MASK_MULADD,  OPCODE_FMSUB(0, 2),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32MSUB< f32, SF0, SF1, SF2, IntConst<int, FE_DOWNWARD> > > > } } },
    { "fmsub.s/rup",    MASK_MULADD,  OPCODE_FMSUB(0, 3),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32MSUB< f32, SF0, SF1, SF2, IntConst<int, FE_UPWARD> > > > } } },
    { "fmsub.s/rmm",    MASK_MULADD,  OPCODE_FMSUB(0, 4),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32MSUB< f32, SF0, SF1, SF2, IntConst<int, FE_TONEAREST> > > > } } },
    { "fmsub.s",        MASK_MULADD,  OPCODE_FMSUB(0, 7),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32MSUB< f32, SF0, SF1, SF2, RISCV32RoundModeFromFCSR > > > } } },

    { "fnmadd.s/rne",   MASK_MULADD,  OPCODE_FNMADD(0, 0),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32NMADD< f32, SF0, SF1, SF2, IntConst<int, FE_TONEAREST> > > > } } },
    { "fnmadd.s/rtz",   MASK_MULADD,  OPCODE_FNMADD(0, 1),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32NMADD< f32, SF0, SF1, SF2, IntConst<int, FE_TOWARDZERO> > > > } } },
    { "fnmadd.s/rdn",   MASK_MULADD,  OPCODE_FNMADD(0, 2),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32NMADD< f32, SF0, SF1, SF2, IntConst<int, FE_DOWNWARD> > > > } } },
    { "fnmadd.s/rup",   MASK_MULADD,  OPCODE_FNMADD(0, 3),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32NMADD< f32, SF0, SF1, SF2, IntConst<int, FE_UPWARD> > > > } } },
    { "fnmadd.s/rmm",   MASK_MULADD,  OPCODE_FNMADD(0, 4),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32NMADD< f32, SF0, SF1, SF2, IntConst<int, FE_TONEAREST> > > > } } },
    { "fnmadd.s",       MASK_MULADD,  OPCODE_FNMADD(0, 7),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32NMADD< f32, SF0, SF1, SF2, RISCV32RoundModeFromFCSR > > > } } },

    { "fnmsub.s/rne",   MASK_MULADD,  OPCODE_FNMSUB(0, 0),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32NMSUB< f32, SF0, SF1, SF2, IntConst<int, FE_TONEAREST> > > > } } },
    { "fnmsub.s/rtz",   MASK_MULADD,  OPCODE_FNMSUB(0, 1),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32NMSUB< f32, SF0, SF1, SF2, IntConst<int, FE_TOWARDZERO> > > > } } },
    { "fnmsub.s/rdn",   MASK_MULADD,  OPCODE_FNMSUB(0, 2),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32NMSUB< f32, SF0, SF1, SF2, IntConst<int, FE_DOWNWARD> > > > } } },
    { "fnmsub.s/rup",   MASK_MULADD,  OPCODE_FNMSUB(0, 3),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32NMSUB< f32, SF0, SF1, SF2, IntConst<int, FE_UPWARD> > > > } } },
    { "fnmsub.s/rmm",   MASK_MULADD,  OPCODE_FNMSUB(0, 4),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32NMSUB< f32, SF0, SF1, SF2, IntConst<int, FE_TONEAREST> > > > } } },
    { "fnmsub.s",       MASK_MULADD,  OPCODE_FNMSUB(0, 7),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   Set< D0, RISCV32NanBoxing< RISCV32NMSUB< f32, SF0, SF1, SF2, RISCV32RoundModeFromFCSR > > > } } },


    // Conversion
    //{Name,            Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    { "fcvt.w.s/rne",   MASK_FCVT,    OPCODE_FCVT(0x60, 0, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< s32, SF0, IntConst<int, FE_TONEAREST> > > } } },
    { "fcvt.w.s/rtz",   MASK_FCVT,    OPCODE_FCVT(0x60, 0, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< s32, SF0, IntConst<int, FE_TOWARDZERO> > > } } },
    { "fcvt.w.s/rdn",   MASK_FCVT,    OPCODE_FCVT(0x60, 0, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< s32, SF0, IntConst<int, FE_DOWNWARD> > > } } },
    { "fcvt.w.s/rup",   MASK_FCVT,    OPCODE_FCVT(0x60, 0, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< s32, SF0, IntConst<int, FE_UPWARD> > > } } },
    { "fcvt.w.s/rmm",   MASK_FCVT,    OPCODE_FCVT(0x60, 0, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< s32, SF0, IntConst<int, FE_TONEAREST> > > } } },
    { "fcvt.w.s",       MASK_FCVT,    OPCODE_FCVT(0x60, 0, 7),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< s32, SF0, RISCV32RoundModeFromFCSR > > } } },

    { "fcvt.wu.s/rne",  MASK_FCVT,    OPCODE_FCVT(0x60, 1, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< u32, SF0, IntConst<int, FE_TONEAREST> > > } } },
    { "fcvt.wu.s/rtz",  MASK_FCVT,    OPCODE_FCVT(0x60, 1, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< u32, SF0, IntConst<int, FE_TOWARDZERO> > > } } },
    { "fcvt.wu.s/rdn",  MASK_FCVT,    OPCODE_FCVT(0x60, 1, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< u32, SF0, IntConst<int, FE_DOWNWARD> > > } } },
    { "fcvt.wu.s/rup",  MASK_FCVT,    OPCODE_FCVT(0x60, 1, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< u32, SF0, IntConst<int, FE_UPWARD> > > } } },
    { "fcvt.wu.s/rmm",  MASK_FCVT,    OPCODE_FCVT(0x60, 1, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< u32, SF0, IntConst<int, FE_TONEAREST> > > } } },
    { "fcvt.wu.s",      MASK_FCVT,    OPCODE_FCVT(0x60, 1, 7),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< u32, SF0, RISCV32RoundModeFromFCSR > > } } },

    { "fcvt.s.w/rne",   MASK_FCVT,    OPCODE_FCVT(0x68, 0, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< CastFP< f32, Cast< s32, S0>, IntConst<int, FE_TONEAREST> > > > } } },
    { "fcvt.s.w/rtz",   MASK_FCVT,    OPCODE_FCVT(0x68, 0, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< CastFP< f32, Cast< s32, S0>, IntConst<int, FE_TOWARDZERO> > > > } } },
    { "fcvt.s.w/rdn",   MASK_FCVT,    OPCODE_FCVT(0x68, 0, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< CastFP< f32, Cast< s32, S0>, IntConst<int, FE_DOWNWARD> > > > } } },
    { "fcvt.s.w/rup",   MASK_FCVT,    OPCODE_FCVT(0x68, 0, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< CastFP< f32, Cast< s32, S0>, IntConst<int, FE_UPWARD> > > > } } },
    { "fcvt.s.w/rmm",   MASK_FCVT,    OPCODE_FCVT(0x68, 0, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< CastFP< f32, Cast< s32, S0>, IntConst<int, FE_TONEAREST> > > > } } },
    { "fcvt.s.w",       MASK_FCVT,    OPCODE_FCVT(0x68, 0, 7),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< CastFP< f32, Cast< s32, S0>, RISCV32RoundModeFromFCSR > > > } } },

    { "fcvt.s.wu/rne",  MASK_FCVT,    OPCODE_FCVT(0x68, 1, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< CastFP< f32, Cast< u32, S0>, IntConst<int, FE_TONEAREST> > > > } } },
    { "fcvt.s.wu/rtz",  MASK_FCVT,    OPCODE_FCVT(0x68, 1, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< CastFP< f32, Cast< u32, S0>, IntConst<int, FE_TOWARDZERO> > > > } } },
    { "fcvt.s.wu/rdn",  MASK_FCVT,    OPCODE_FCVT(0x68, 1, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< CastFP< f32, Cast< u32, S0>, IntConst<int, FE_DOWNWARD> > > > } } },
    { "fcvt.s.wu/rup",  MASK_FCVT,    OPCODE_FCVT(0x68, 1, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< CastFP< f32, Cast< u32, S0>, IntConst<int, FE_UPWARD> > > > } } },
    { "fcvt.s.wu/rmm",  MASK_FCVT,    OPCODE_FCVT(0x68, 1, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< CastFP< f32, Cast< u32, S0>, IntConst<int, FE_TONEAREST> > > > } } },
    { "fcvt.s.wu",      MASK_FCVT,    OPCODE_FCVT(0x68, 1, 7),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< CastFP< f32, Cast< u32, S0>, RISCV32RoundModeFromFCSR > > > } } },

    // Move
    //{Name,        Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    { "fsgnj.s",    MASK_FLOAT,   OPCODE_FLOAT(0x10, 0),    1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< AsFP< f32, FPFLOATCopySign<S1, S0> > > > } } },
    { "fsgnjn.s",   MASK_FLOAT,   OPCODE_FLOAT(0x10, 1),    1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< AsFP< f32, FPFLOATCopySignNeg<S1, S0> > > > } } },
    { "fsgnjx.s",   MASK_FLOAT,   OPCODE_FLOAT(0x10, 2),    1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32NanBoxing< AsFP< f32, FPFLOATCopySignXor<S1, S0> > > > } } },
    { "fmv.x.w",    MASK_MV,      OPCODE_MV(0x70),          1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, -1, -1, -1},   Set< D0, AsInt<u32, SF0> > } } },
    { "fmv.w.x",    MASK_MV,      OPCODE_MV(0x78),          1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32NanBoxing< AsFP< f32, S0> > > } } },

    // Compare
    //{Name,        Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    { "feq.s",      MASK_FLOAT,   OPCODE_FLOAT(0x50, 2),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32FPEqual< f32, SF0, SF1> > } } },
    { "flt.s",      MASK_FLOAT,   OPCODE_FLOAT(0x50, 1),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32FPLess< f32, SF0, SF1> > } } },
    { "fle.s",      MASK_FLOAT,   OPCODE_FLOAT(0x50, 0),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32FPLessEqual< f32, SF0, SF1> > } } },

    // Class
    //{Name,        Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    { "fclass.s",   MASK_FCLASS,  OPCODE_FCLASS(0x70),      1,{ { OpClassCode::fADD,   {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FCLASS< f32, SF0> > } } },


    // RV32D

    // LOAD/STORE
    //{Name,    Mask,       Opcode,           nOp,{ OpClassCode,        Dst[],      Src[],               OpInfoType::EmulationFunc}[]}
    { "fld",    MASK_LD,    OPCODE_FLD(3),    1,{ { OpClassCode::fLD,   {R0, -1},   {R1, I0, -1, -1},    Set<D0, Load<u64, RISCV32Addr<S0, S1> > > } } },
    { "fsd",    MASK_ST,    OPCODE_FST(3),    1,{ { OpClassCode::fST,   {-1, -1},   {R1, R0, I0, -1},    Store<u64, S0, RISCV32Addr<S1, S2> > } } },

    // FLOAT
    //{Name,            Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    { "fadd.d/rne",     MASK_FLOAT,   OPCODE_FLOAT(0x01, 0),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPAdd< f64, SD0, SD1, IntConst<int, FE_TONEAREST> > > } } },
    { "fadd.d/rtz",     MASK_FLOAT,   OPCODE_FLOAT(0x01, 1),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPAdd< f64, SD0, SD1, IntConst<int, FE_TOWARDZERO> > > } } },
    { "fadd.d/rdn",     MASK_FLOAT,   OPCODE_FLOAT(0x01, 2),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPAdd< f64, SD0, SD1, IntConst<int, FE_DOWNWARD> > > } } },
    { "fadd.d/rup",     MASK_FLOAT,   OPCODE_FLOAT(0x01, 3),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPAdd< f64, SD0, SD1, IntConst<int, FE_UPWARD> > > } } },
    { "fadd.d/rmm",     MASK_FLOAT,   OPCODE_FLOAT(0x01, 4),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPAdd< f64, SD0, SD1, IntConst<int, FE_TONEAREST> > > } } },
    { "fadd.d",         MASK_FLOAT,   OPCODE_FLOAT(0x01, 7),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPAdd< f64, SD0, SD1, RISCV32RoundModeFromFCSR > > } } },

    { "fsub.d/rne",     MASK_FLOAT,   OPCODE_FLOAT(0x05, 0),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPSub< f64, SD0, SD1, IntConst<int, FE_TONEAREST> > > } } },
    { "fsub.d/rtz",     MASK_FLOAT,   OPCODE_FLOAT(0x05, 1),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPSub< f64, SD0, SD1, IntConst<int, FE_TOWARDZERO> > > } } },
    { "fsub.d/rdn",     MASK_FLOAT,   OPCODE_FLOAT(0x05, 2),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPSub< f64, SD0, SD1, IntConst<int, FE_DOWNWARD> > > } } },
    { "fsub.d/rup",     MASK_FLOAT,   OPCODE_FLOAT(0x05, 3),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPSub< f64, SD0, SD1, IntConst<int, FE_UPWARD> > > } } },
    { "fsub.d/rmm",     MASK_FLOAT,   OPCODE_FLOAT(0x05, 4),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPSub< f64, SD0, SD1, IntConst<int, FE_TONEAREST> > > } } },
    { "fsub.d",         MASK_FLOAT,   OPCODE_FLOAT(0x05, 7),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPSub< f64, SD0, SD1, RISCV32RoundModeFromFCSR > > } } },

    { "fmul.d/rne",     MASK_FLOAT,   OPCODE_FLOAT(0x09, 0),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPMul< f64, SD0, SD1, IntConst<int, FE_TONEAREST> > > } } },
    { "fmul.d/rtz",     MASK_FLOAT,   OPCODE_FLOAT(0x09, 1),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPMul< f64, SD0, SD1, IntConst<int, FE_TOWARDZERO> > > } } },
    { "fmul.d/rdn",     MASK_FLOAT,   OPCODE_FLOAT(0x09, 2),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPMul< f64, SD0, SD1, IntConst<int, FE_DOWNWARD> > > } } },
    { "fmul.d/rup",     MASK_FLOAT,   OPCODE_FLOAT(0x09, 3),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPMul< f64, SD0, SD1, IntConst<int, FE_UPWARD> > > } } },
    { "fmul.d/rmm",     MASK_FLOAT,   OPCODE_FLOAT(0x09, 4),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPMul< f64, SD0, SD1, IntConst<int, FE_TONEAREST> > > } } },
    { "fmul.d",         MASK_FLOAT,   OPCODE_FLOAT(0x09, 7),    1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPMul< f64, SD0, SD1, RISCV32RoundModeFromFCSR > > } } },

    { "fdiv.d/rne",     MASK_FLOAT,   OPCODE_FLOAT(0x0d, 0),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPDiv< f64, SD0, SD1, IntConst<int, FE_TONEAREST> > > } } },
    { "fdiv.d/rtz",     MASK_FLOAT,   OPCODE_FLOAT(0x0d, 1),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPDiv< f64, SD0, SD1, IntConst<int, FE_TOWARDZERO> > > } } },
    { "fdiv.d/rdn",     MASK_FLOAT,   OPCODE_FLOAT(0x0d, 2),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPDiv< f64, SD0, SD1, IntConst<int, FE_DOWNWARD> > > } } },
    { "fdiv.d/rup",     MASK_FLOAT,   OPCODE_FLOAT(0x0d, 3),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPDiv< f64, SD0, SD1, IntConst<int, FE_UPWARD> > > } } },
    { "fdiv.d/rmm",     MASK_FLOAT,   OPCODE_FLOAT(0x0d, 4),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPDiv< f64, SD0, SD1, IntConst<int, FE_TONEAREST> > > } } },
    { "fdiv.d",         MASK_FLOAT,   OPCODE_FLOAT(0x0d, 7),    1,{ { OpClassCode::fDIV,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, FPDiv< f64, SD0, SD1, RISCV32RoundModeFromFCSR > > } } },

    { "fsqrt.d/rne",    MASK_SQRT,    OPCODE_SQRT(0x2d, 0),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, FPSqrt< f64, SD0, IntConst<int, FE_TONEAREST> > > } } },
    { "fsqrt.d/rtz",    MASK_SQRT,    OPCODE_SQRT(0x2d, 1),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, FPSqrt< f64, SD0, IntConst<int, FE_TOWARDZERO> > > } } },
    { "fsqrt.d/rdn",    MASK_SQRT,    OPCODE_SQRT(0x2d, 2),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, FPSqrt< f64, SD0, IntConst<int, FE_DOWNWARD> > > } } },
    { "fsqrt.d/rup",    MASK_SQRT,    OPCODE_SQRT(0x2d, 3),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, FPSqrt< f64, SD0, IntConst<int, FE_UPWARD> > > } } },
    { "fsqrt.d/rmm",    MASK_SQRT,    OPCODE_SQRT(0x2d, 4),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, FPSqrt< f64, SD0, IntConst<int, FE_TONEAREST> > > } } },
    { "fsqrt.d",        MASK_SQRT,    OPCODE_SQRT(0x2d, 7),     1,{ { OpClassCode::fELEM,  {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, FPSqrt< f64, SD0, RISCV32RoundModeFromFCSR > > } } },

    { "fmin.d",         MASK_FLOAT,   OPCODE_FLOAT(0x15, 0),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, RISCV32FPMIN< f64, SD0, SD1> > } } },
    { "fmax.d",         MASK_FLOAT,   OPCODE_FLOAT(0x15, 1),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   SetFP< D0, RISCV32FPMAX< f64, SD0, SD1> > } } },

    { "fmadd.d/rtz",    MASK_MULADD,  OPCODE_FMADD(1, 1),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32MADD< f64, SD0, SD1, SD2, IntConst<int, FE_TOWARDZERO> > > } } },
    { "fmadd.d/rdn",    MASK_MULADD,  OPCODE_FMADD(1, 2),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32MADD< f64, SD0, SD1, SD2, IntConst<int, FE_DOWNWARD> > > } } },
    { "fmadd.d/rne",    MASK_MULADD,  OPCODE_FMADD(1, 0),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32MADD< f64, SD0, SD1, SD2, IntConst<int, FE_TONEAREST> > > } } },
    { "fmadd.d/rup",    MASK_MULADD,  OPCODE_FMADD(1, 3),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32MADD< f64, SD0, SD1, SD2, IntConst<int, FE_UPWARD> > > } } },
    { "fmadd.d/rmm",    MASK_MULADD,  OPCODE_FMADD(1, 4),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32MADD< f64, SD0, SD1, SD2, IntConst<int, FE_TONEAREST> > > } } },
    { "fmadd.d",        MASK_MULADD,  OPCODE_FMADD(1, 7),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32MADD< f64, SD0, SD1, SD2, RISCV32RoundModeFromFCSR > > } } },

    { "fmsub.d/rne",    MASK_MULADD,  OPCODE_FMSUB(1, 0),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32MSUB< f64, SD0, SD1, SD2, IntConst<int, FE_TONEAREST> > > } } },
    { "fmsub.d/rtz",    MASK_MULADD,  OPCODE_FMSUB(1, 1),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32MSUB< f64, SD0, SD1, SD2, IntConst<int, FE_TOWARDZERO> > > } } },
    { "fmsub.d/rdn",    MASK_MULADD,  OPCODE_FMSUB(1, 2),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32MSUB< f64, SD0, SD1, SD2, IntConst<int, FE_DOWNWARD> > > } } },
    { "fmsub.d/rup",    MASK_MULADD,  OPCODE_FMSUB(1, 3),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32MSUB< f64, SD0, SD1, SD2, IntConst<int, FE_UPWARD> > > } } },
    { "fmsub.d/rmm",    MASK_MULADD,  OPCODE_FMSUB(1, 4),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32MSUB< f64, SD0, SD1, SD2, IntConst<int, FE_TONEAREST> > > } } },
    { "fmsub.d",        MASK_MULADD,  OPCODE_FMSUB(1, 7),       1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32MSUB< f64, SD0, SD1, SD2, RISCV32RoundModeFromFCSR > > } } },

    { "fnmadd.d/rne",   MASK_MULADD,  OPCODE_FNMADD(1, 0),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32NMADD< f64, SD0, SD1, SD2, IntConst<int, FE_TONEAREST> > > } } },
    { "fnmadd.d/rtz",   MASK_MULADD,  OPCODE_FNMADD(1, 1),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32NMADD< f64, SD0, SD1, SD2, IntConst<int, FE_TOWARDZERO> > > } } },
    { "fnmadd.d/rdn",   MASK_MULADD,  OPCODE_FNMADD(1, 2),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32NMADD< f64, SD0, SD1, SD2, IntConst<int, FE_DOWNWARD> > > } } },
    { "fnmadd.d/rup",   MASK_MULADD,  OPCODE_FNMADD(1, 3),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32NMADD< f64, SD0, SD1, SD2, IntConst<int, FE_UPWARD> > > } } },
    { "fnmadd.d/rmm",   MASK_MULADD,  OPCODE_FNMADD(1, 4),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32NMADD< f64, SD0, SD1, SD2, IntConst<int, FE_TONEAREST> > > } } },
    { "fnmadd.d",       MASK_MULADD,  OPCODE_FNMADD(1, 7),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32NMADD< f64, SD0, SD1, SD2, RISCV32RoundModeFromFCSR > > } } },

    { "fnmsub.d/rne",   MASK_MULADD,  OPCODE_FNMSUB(1, 0),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32NMSUB< f64, SD0, SD1, SD2, IntConst<int, FE_TONEAREST> > > } } },
    { "fnmsub.d/rtz",   MASK_MULADD,  OPCODE_FNMSUB(1, 1),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32NMSUB< f64, SD0, SD1, SD2, IntConst<int, FE_TOWARDZERO> > > } } },
    { "fnmsub.d/rdn",   MASK_MULADD,  OPCODE_FNMSUB(1, 2),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32NMSUB< f64, SD0, SD1, SD2, IntConst<int, FE_DOWNWARD> > > } } },
    { "fnmsub.d/rup",   MASK_MULADD,  OPCODE_FNMSUB(1, 3),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32NMSUB< f64, SD0, SD1, SD2, IntConst<int, FE_UPWARD> > > } } },
    { "fnmsub.d/rmm",   MASK_MULADD,  OPCODE_FNMSUB(1, 4),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32NMSUB< f64, SD0, SD1, SD2, IntConst<int, FE_TONEAREST> > > } } },
    { "fnmsub.d",       MASK_MULADD,  OPCODE_FNMSUB(1, 7),      1,{ { OpClassCode::fMUL,   {R0, -1},   {R1, R2, R3, -1},   SetFP< D0, RISCV32NMSUB< f64, SD0, SD1, SD2, RISCV32RoundModeFromFCSR > > } } },

    // Conversion
    //{Name,            Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    { "fcvt.w.d/rne",   MASK_FCVT,    OPCODE_FCVT(0x61, 0, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< s32, SD0, IntConst<int, FE_TONEAREST> > > } } },
    { "fcvt.w.d/rtz",   MASK_FCVT,    OPCODE_FCVT(0x61, 0, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< s32, SD0, IntConst<int, FE_TOWARDZERO> > > } } },
    { "fcvt.w.d/rdn",   MASK_FCVT,    OPCODE_FCVT(0x61, 0, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< s32, SD0, IntConst<int, FE_DOWNWARD> > > } } },
    { "fcvt.w.d/rup",   MASK_FCVT,    OPCODE_FCVT(0x61, 0, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< s32, SD0, IntConst<int, FE_UPWARD> > > } } },
    { "fcvt.w.d/rmm",   MASK_FCVT,    OPCODE_FCVT(0x61, 0, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< s32, SD0, IntConst<int, FE_TONEAREST> > > } } },
    { "fcvt.w.d",       MASK_FCVT,    OPCODE_FCVT(0x61, 0, 7),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< s32, SD0, RISCV32RoundModeFromFCSR > > } } },

    { "fcvt.wu.d/rne",  MASK_FCVT,    OPCODE_FCVT(0x61, 1, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< u32, SD0, IntConst<int, FE_TONEAREST> > > } } },
    { "fcvt.wu.d/rtz",  MASK_FCVT,    OPCODE_FCVT(0x61, 1, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< u32, SD0, IntConst<int, FE_TOWARDZERO> > > } } },
    { "fcvt.wu.d/rdn",  MASK_FCVT,    OPCODE_FCVT(0x61, 1, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< u32, SD0, IntConst<int, FE_DOWNWARD> > > } } },
    { "fcvt.wu.d/rup",  MASK_FCVT,    OPCODE_FCVT(0x61, 1, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< u32, SD0, IntConst<int, FE_UPWARD> > > } } },
    { "fcvt.wu.d/rmm",  MASK_FCVT,    OPCODE_FCVT(0x61, 1, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< u32, SD0, IntConst<int, FE_TONEAREST> > > } } },
    { "fcvt.wu.d",      MASK_FCVT,    OPCODE_FCVT(0x61, 1, 7),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FPToInt< u32, SD0, RISCV32RoundModeFromFCSR > > } } },

    { "fcvt.d.w/rne",   MASK_FCVT,    OPCODE_FCVT(0x69, 0, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< s32, S0>, IntConst<int, FE_TONEAREST> > > } } },
    { "fcvt.d.w/rtz",   MASK_FCVT,    OPCODE_FCVT(0x69, 0, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< s32, S0>, IntConst<int, FE_TOWARDZERO> > > } } },
    { "fcvt.d.w/rdn",   MASK_FCVT,    OPCODE_FCVT(0x69, 0, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< s32, S0>, IntConst<int, FE_DOWNWARD> > > } } },
    { "fcvt.d.w/rup",   MASK_FCVT,    OPCODE_FCVT(0x69, 0, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< s32, S0>, IntConst<int, FE_UPWARD> > > } } },
    { "fcvt.d.w/rmm",   MASK_FCVT,    OPCODE_FCVT(0x69, 0, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< s32, S0>, IntConst<int, FE_TONEAREST> > > } } },
    { "fcvt.d.w",       MASK_FCVT,    OPCODE_FCVT(0x69, 0, 7),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< s32, S0>, RISCV32RoundModeFromFCSR > > } } },

    { "fcvt.d.wu/rne",  MASK_FCVT,    OPCODE_FCVT(0x69, 1, 0),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< u32, S0>, IntConst<int, FE_TONEAREST> > > } } },
    { "fcvt.d.wu/rtz",  MASK_FCVT,    OPCODE_FCVT(0x69, 1, 1),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< u32, S0>, IntConst<int, FE_TOWARDZERO> > > } } },
    { "fcvt.d.wu/rdn",  MASK_FCVT,    OPCODE_FCVT(0x69, 1, 2),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< u32, S0>, IntConst<int, FE_DOWNWARD> > > } } },
    { "fcvt.d.wu/rup",  MASK_FCVT,    OPCODE_FCVT(0x69, 1, 3),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< u32, S0>, IntConst<int, FE_UPWARD> > > } } },
    { "fcvt.d.wu/rmm",  MASK_FCVT,    OPCODE_FCVT(0x69, 1, 4),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< u32, S0>, IntConst<int, FE_TONEAREST> > > } } },
    { "fcvt.d.wu",      MASK_FCVT,    OPCODE_FCVT(0x69, 1, 7),  1,{ { OpClassCode::ifCONV, {R0, -1},   {R1, -1, -1, -1},   SetFP< D0, CastFP< f64, Cast< u32, S0>, RISCV32RoundModeFromFCSR > > } } },

    // FCVT.D.S instruction will never round, but have RM field.
    { "fcvt.s.d/rne",   MASK_FCVT,    OPCODE_FCVT(0x20, 1, 0),  1,{ { OpClassCode::fCONV, {R0, -1},   {R1, -1, -1, -1},    Set< D0, RISCV32NanBoxing< CastFP< f32, SD0, IntConst<int, FE_TONEAREST> > > > } } },
    { "fcvt.s.d/rtz",   MASK_FCVT,    OPCODE_FCVT(0x20, 1, 1),  1,{ { OpClassCode::fCONV, {R0, -1},   {R1, -1, -1, -1},    Set< D0, RISCV32NanBoxing< CastFP< f32, SD0, IntConst<int, FE_TOWARDZERO> > > > } } },
    { "fcvt.s.d/rdn",   MASK_FCVT,    OPCODE_FCVT(0x20, 1, 2),  1,{ { OpClassCode::fCONV, {R0, -1},   {R1, -1, -1, -1},    Set< D0, RISCV32NanBoxing< CastFP< f32, SD0, IntConst<int, FE_DOWNWARD> > > > } } },
    { "fcvt.s.d/rup",   MASK_FCVT,    OPCODE_FCVT(0x20, 1, 3),  1,{ { OpClassCode::fCONV, {R0, -1},   {R1, -1, -1, -1},    Set< D0, RISCV32NanBoxing< CastFP< f32, SD0, IntConst<int, FE_UPWARD> > > > } } },
    { "fcvt.s.d/rmm",   MASK_FCVT,    OPCODE_FCVT(0x20, 1, 4),  1,{ { OpClassCode::fCONV, {R0, -1},   {R1, -1, -1, -1},    Set< D0, RISCV32NanBoxing< CastFP< f32, SD0, IntConst<int, FE_TONEAREST> > > > } } },
    { "fcvt.s.d",       MASK_FCVT,    OPCODE_FCVT(0x20, 1, 7),  1,{ { OpClassCode::fCONV, {R0, -1},   {R1, -1, -1, -1},    Set< D0, RISCV32NanBoxing< CastFP< f32, SD0, RISCV32RoundModeFromFCSR > > > } } },

    { "fcvt.d.s/rne",   MASK_FCVT,    OPCODE_FCVT(0x21, 0, 0),  1,{ { OpClassCode::fCONV, {R0, -1},   {R1, -1, -1, -1},    SetFP< D0, CastFP< f64, SF0, IntConst<int, FE_TONEAREST> > > } } },
    { "fcvt.d.s/rtz",   MASK_FCVT,    OPCODE_FCVT(0x21, 0, 1),  1,{ { OpClassCode::fCONV, {R0, -1},   {R1, -1, -1, -1},    SetFP< D0, CastFP< f64, SF0, IntConst<int, FE_TOWARDZERO> > > } } },
    { "fcvt.d.s/rdn",   MASK_FCVT,    OPCODE_FCVT(0x21, 0, 2),  1,{ { OpClassCode::fCONV, {R0, -1},   {R1, -1, -1, -1},    SetFP< D0, CastFP< f64, SF0, IntConst<int, FE_DOWNWARD> > > } } },
    { "fcvt.d.s/rup",   MASK_FCVT,    OPCODE_FCVT(0x21, 0, 3),  1,{ { OpClassCode::fCONV, {R0, -1},   {R1, -1, -1, -1},    SetFP< D0, CastFP< f64, SF0, IntConst<int, FE_UPWARD> > > } } },
    { "fcvt.d.s/rmm",   MASK_FCVT,    OPCODE_FCVT(0x21, 0, 4),  1,{ { OpClassCode::fCONV, {R0, -1},   {R1, -1, -1, -1},    SetFP< D0, CastFP< f64, SF0, IntConst<int, FE_TONEAREST> > > } } },
    { "fcvt.d.s",       MASK_FCVT,    OPCODE_FCVT(0x21, 0, 7),  1,{ { OpClassCode::fCONV, {R0, -1},   {R1, -1, -1, -1},    SetFP< D0, CastFP< f64, SF0, RISCV32RoundModeFromFCSR > > } } },

    // Move
    //{Name,        Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    { "fsgnj.d",    MASK_FLOAT,   OPCODE_FLOAT(0x11, 0),    1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, FPDoubleCopySign<S1, S0> > } } },
    { "fsgnjn.d",   MASK_FLOAT,   OPCODE_FLOAT(0x11, 1),    1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, FPDoubleCopySignNeg<S1, S0> > } } },
    { "fsgnjx.d",   MASK_FLOAT,   OPCODE_FLOAT(0x11, 2),    1,{ { OpClassCode::fMOV,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, FPDoubleCopySignXor<S1, S0> > } } },


    // Compare
    //{Name,        Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    { "feq.d",      MASK_FLOAT,   OPCODE_FLOAT(0x51, 2),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32FPEqual< f64, SD0, SD1> > } } },
    { "flt.d",      MASK_FLOAT,   OPCODE_FLOAT(0x51, 1),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32FPLess< f64, SD0, SD1> > } } },
    { "fle.d",      MASK_FLOAT,   OPCODE_FLOAT(0x51, 0),    1,{ { OpClassCode::fADD,   {R0, -1},   {R1, R2, -1, -1},   Set< D0, RISCV32FPLessEqual< f64, SD0, SD1> > } } },

    // Class
    //{Name,        Mask,         Opcode,                   nOp,{ OpClassCode,         Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    { "fclass.d",   MASK_FCLASS,  OPCODE_FCLASS(0x71),      1,{ { OpClassCode::fADD,   {R0, -1},   {R1, -1, -1, -1},   Set< D0, RISCV32FCLASS< f64, SD0> > } } },

  // pseudo instruction (special case of addi)
    {"nop",   MASK_EXACT, OPCODE_IMM(0), 1, { {OpClassCode::iNOP, {R0, -1}, {R1, I0, -1, -1}, NoOperation} } },
    {"mv",    MASK_IMM | (0xfff << 20), OPCODE_IMM(0), 1, { {OpClassCode::iMOV, {R0, -1}, {R1, I0, -1, -1}, Set<D0, S0>} } },

};

//
// RISCV32Converter
//


RISCV32Converter::RISCV32Converter()
{
    AddToOpMap(m_OpDefsBase, sizeof(m_OpDefsBase)/sizeof(OpDef));
}

RISCV32Converter::~RISCV32Converter()
{
}

// srcTemplate に対応するオペランドの種類と，レジスタならば番号を，即値ならばindexを返す
std::pair<RISCV32Converter::OperandType, int> RISCV32Converter::GetActualSrcOperand(int srcTemplate, const DecodedInsn& decoded) const
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
int RISCV32Converter::GetActualRegNumber(int regTemplate, const DecodedInsn& decoded) const
{
    if (regTemplate == -1) {
        return -1;
    }
    else if (RegTemplateBegin <= regTemplate && regTemplate <= RegTemplateEnd) {
        return decoded.Reg[regTemplate - RegTemplateBegin];
    }
    else if (0 <= regTemplate && regTemplate < RISCV32Info::RegisterCount) {
        return regTemplate;
    }
    else {
        ASSERT(0, "RISCV32Converter Logic Error : invalid regTemplate");
        return -1;
    }
}

// レジスタ番号regNumがゼロレジスタかどうかを判定する
bool RISCV32Converter::IsZeroReg(int regNum) const
{
    const int IntZeroReg = 0;
    return regNum == IntZeroReg;    // FP にはゼロレジスタは存在しない
}


void RISCV32Converter::RISCVUnknownOperation(OpEmulationState* opState)
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

const RISCV32Converter::OpDef* RISCV32Converter::GetOpDefUnknown() const
{
    return &m_OpDefUnknown;
}
