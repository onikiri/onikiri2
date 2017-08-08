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
    
    static const u32 MASK_JAL =     0x0000007f; // J-type
    static const u32 MASK_J =       0x00000fff; // J-type, rd

    static const u32 MASK_JALR =    0x0000707f; // J-type?, 
    static const u32 MASK_CALLRET = 0x000fffff; // J-type, rs1, funct3, rd, opcode
    static const u32 MASK_CALL =    0x00007fff; // J-type, funct3, rd, opcode
    static const u32 MASK_RET =     0x000ff07f; // J-type, rs1, funct3, rd, opcode

    static const u32 MASK_BR  =     0x0000707f; // B-type, funct3

    static const u32 MASK_ST  =     0x0000707f; // B-type, funct3
    static const u32 MASK_LD  =     0x0000707f; // B-type, funct3
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

    const int I0 = ImmTemplateBegin+0;
    const int I1 = ImmTemplateBegin+1;

    const int T0 = RISCV32Info::REG_ADDRESS;
    const int FPC = RISCV32Info::REG_FPCR;
}

#define RISCV32_DSTOP(n) RISCV32DstOperand<n>
#define RISCV32_SRCOP(n) RISCV32SrcOperand<n>
//#define RISCV32_SRCOPFLOAT(n) Cast< float, AsFP< double, SrcOperand<n> > >
//#define RISCV32_SRCOPDOUBLE(n) AsFP< double, SrcOperand<n> >

#define D0 RISCV32_DSTOP(0)
#define D1 RISCV32_DSTOP(1)
#define S0 RISCV32_SRCOP(0)
#define S1 RISCV32_SRCOP(1)
#define S2 RISCV32_SRCOP(2)
#define S3 RISCV32_SRCOP(3)
/*
#define SF0 RISCV32_SRCOPFLOAT(0)
#define SF1 RISCV32_SRCOPFLOAT(1)
#define SF2 RISCV32_SRCOPFLOAT(2)
#define SF3 RISCV32_SRCOPFLOAT(3)
#define SD0 RISCV32_SRCOPDOUBLE(0)
#define SD1 RISCV32_SRCOPDOUBLE(1)
#define SD2 RISCV32_SRCOPDOUBLE(2)
#define SD3 RISCV32_SRCOPDOUBLE(3)
*/
// 00000000 PAL 00 = halt
// 2ffe0000 ldq_u r31, r30(0) = unop
// 471f041f mslql r31, r31, r31 = nop

// no trap implemented

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
    {"slli",    MASK_SHIFT, OPCODE_SHIFT(0x00, 1),  1,  { {OpClassCode::iALU,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, LShiftL<u32, S0, S1, 0x1f > > } } },
    {"srli",    MASK_SHIFT, OPCODE_SHIFT(0x00, 5),  1,  { {OpClassCode::iALU,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, LShiftR<u32, S0, S1, 0x1f > > } } },
    {"srai",    MASK_SHIFT, OPCODE_SHIFT(0x20, 5),  1,  { {OpClassCode::iALU,   {R0, -1},   {R1, I0, -1, -1},   Set<D0, AShiftR<u32, S0, S1, 0x1f > > } } },

    // INT
    //{Name,    Mask,       Opcode,                 nOp,{ OpClassCode,          Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"add",     MASK_INT,   OPCODE_INT(0x00, 0),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, IntAdd<u32, S0, S1> > } } },
    {"sub",     MASK_INT,   OPCODE_INT(0x20, 0),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, IntSub<u32, S0, S1> > } } },
    {"sll",     MASK_INT,   OPCODE_INT(0x00, 1),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, LShiftL<u32, S0, S1, 0x1f > > } } },
    {"slt",     MASK_INT,   OPCODE_INT(0x00, 2),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, RISCV32Compare<S0, S1, IntCondLessSigned<u32> > > } } },
    {"sltu",    MASK_INT,   OPCODE_INT(0x00, 3),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, RISCV32Compare<S0, S1, IntCondLessUnsigned<u32> > > } } },
    {"xor",     MASK_INT,   OPCODE_INT(0x00, 4),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, BitXor<u32, S0, S1> > } } },
    {"srl",     MASK_INT,   OPCODE_INT(0x00, 5),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, LShiftR<u32, S0, S1, 0x1f > > } } },
    {"sra",     MASK_INT,   OPCODE_INT(0x20, 5),    1,  { {OpClassCode::iALU,   {R0, -1},   {R1, R2, -1, -1},   Set<D0, AShiftR<u32, S0, S1, 0x1f > > } } },
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
    //    link  link   0        push and pop
    //    link  link   1        push    //{Name,    Mask,           Opcode,                 nOp,{ OpClassCode,              Dst[],      Src[],              OpInfoType::EmulationFunc}[]}
    {"jalr",    MASK_CALLRET,   OPCODE_CALLRET(1, 5),   1,  { { OpClassCode::iJUMP,     {R0, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_CALLRET,   OPCODE_CALLRET(5, 1),   1,  { { OpClassCode::iJUMP,     {R0, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_CALL,      OPCODE_CALL(5),         1,  { { OpClassCode::CALL_JUMP, {R0, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_CALL,      OPCODE_CALL(1),         1,  { { OpClassCode::CALL_JUMP, {R0, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_RET,       OPCODE_RET(5),          1,  { { OpClassCode::RET,       {R0, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_RET,       OPCODE_RET(1),          1,  { { OpClassCode::RET,       {R0, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_CALL,      OPCODE_CALL(0),         1,  { { OpClassCode::iJUMP,     {-1, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },
    {"jalr",    MASK_JALR,      OPCODE_JALR(),          1,  { { OpClassCode::CALL_JUMP, {R0, -1},   {R1, I0, -1, -1},   RISCV32CallAbsUncond<D0, S0, S1> } } },

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
    const int FPZeroReg = 32;

    return regNum == IntZeroReg || regNum == FPZeroReg;
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
