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

#include "Emu/PPC64Linux/PPC64Converter.h"

#include "Emu/Utility/GenericOperation.h"
#include "Emu/Utility/OpEmulationState.h"

#include "Emu/PPC64Linux/PPC64Info.h"
#include "Emu/PPC64Linux/PPC64OpInfo.h"
#include "Emu/PPC64Linux/PPC64Decoder.h"
#include "Emu/PPC64Linux/PPC64Operation.h"

using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::PPC64Linux;
using namespace Onikiri::EmulatorUtility::Operation;
using namespace Onikiri::PPC64Linux::Operation;

//
// 命令の定義
//
//
namespace {
    // XO = extra opcode

    // 各命令形式に対するオペコードを得るためのマスク (0のビットが引数)
    const u32 MASK_EXACT  = 0xffffffff; // 全bitが一致
    const u32 MASK_OP     = 0xfc000000; // opcode のみ
    const u32 MASK_OPF    = 0xfc000001; // opcode と Rc
    const u32 MASK_B      = 0xfc000003; // I-Form unconditional branch
    const u32 MASK_MD     = 0xfc00001d; // MD-Form (opcode, 3-bit XO, Rc)
    const u32 MASK_MDS    = 0xfc00001f; // MDS-Form (opcode, 4-bit XO, Rc)
    const u32 MASK_X2     = 0xfc000003; // 2ビットのXOのみ (DS-Form)
    const u32 MASK_XS9    = 0xfc0007fc; // XS-Form (9-bit XO)
//  const u32 MASK_X10SH  = 0xfc0007fc; // XO10 - sh (XO のうち下位1ビットを sh に使用)
    const u32 MASK_X10    = 0xfc0007fe; // 10ビットのXOのみ

//  const u32 MASK_X10LK  = 0xfc0007ff; // XO10 + LK (Link Register write)

    const u32 MASK_X10OE  = 0xfc0003fe; // XO10 - OE (XO のうち上位1ビットを OE に使用)
    const u32 MASK_X10MB  = 0xfc00001e; // XO10 - MB (XO のうち上位6ビットを MB/ME に使用)
    const u32 MASK_X10MBSH = 0xfc00001c;    // XO10 - MB - SH

    const u32 MASK_X5F    = 0xfc00003f; // XO5 + Rc (FP A-Form instructions)
    const u32 MASK_X10F   = 0xfc0007ff; // XO10 + Rc (flag write)
    const u32 MASK_X10FOE = 0xfc0003ff; // XO10 - OE + Rc
    const u32 MASK_X10FSH = 0xfc0007fd; // XO10 - sh + Rc
    const u32 MASK_X10FMB = 0xfc00001f; // XO10 - MB (XO のうち上位6ビットを MB/ME に使用)
    const u32 MASK_X10FMBSH = 0xfc00001d;   // XO10 - MB - SH + Rc

    const u32 MASK_MTFSPR = 0xfc1ffffe; // mtspr/mfspr
    const u32 MASK_MTCRF  = 0xfc1ff7fe; // mtcrf
    const u32 MASK_DCMP   = 0xfc200000; // D-Form compare (opcode, 32/64-bit flag)
    const u32 MASK_X10CMP = 0xfc2007fe; // X-Form compare (opcode, 32/64-bit flag, 10-bit XO)

    const u32 MASK_RA     = 0x001f0000; // RAの位置 (RA=0 を特別扱いする場合に使用)
}
#define MASK_BC(mask_bo) (MASK_B | (mask_bo) << 21)
#define MASK_BCLR(mask_bo) (MASK_X10 | (mask_bo) << 21 | 0x3 << 11 | 0x1)

// B-Form conditional branch
#define OPCODE_OP(c) (u32)((c) << 26)
#define OPCODE_OPF(c, f) (u32)(OPCODE_OP(c) | (f))
#define OPCODE_B(c, aa, lk) (u32)(OPCODE_OP(c) | (aa) << 1 | (lk))
#define OPCODE_BC(c, aa, lk, bo) (u32)(OPCODE_B(c, aa, lk) | (bo) << 21)

#define OPCODE_MD(c, x, f) (u32)(OPCODE_OP(c) | (x) << 2 | (f))
#define OPCODE_MDS(c, x, f) (u32)(OPCODE_OP(c) | (x) << 1 | (f))
#define OPCODE_X2(c, x) (u32)(OPCODE_OP(c) | (x))
#define OPCODE_X5(c, x) (u32)(OPCODE_OP(c) | (x) << 1)
#define OPCODE_XS9(c, x) (u32)(OPCODE_OP(c) | (x) << 2)
#define OPCODE_X10(c, x) (u32)(OPCODE_OP(c) | (x) << 1)
#define OPCODE_X10OE(c, x, oe) (u32)(OPCODE_X10(c,x) | (oe) << 10)
#define OPCODE_X10FOE(c, x, oe, f) (u32)(OPCODE_X10OE(c,x,oe) | (f))
#define OPCODE_X5F(c, x, f) (u32)(OPCODE_X5(c, x) | (f))
#define OPCODE_XS9F(c, x, f) (u32)(OPCODE_XS9(c, x) | (f))
#define OPCODE_X10F(c, x, f) (u32)(OPCODE_X10(c, x) | (f))

#define OPCODE_BCLR(c, xo, lk, bo, bh) (u32)(OPCODE_X10(c, xo) | (bo) << 21 | (bh) << 11 | (lk))

#define OPCODE_MTFSPR(c, xo, spr) (u32)(OPCODE_X10(c, xo) | ((spr) & 0x01f) << 16 | ((spr) & 0x3e0) >> 5 << 11)
#define OPCODE_DCMP(c, L) (u32)(OPCODE_OP(c) | (L) << 21)
#define OPCODE_X10CMP(c, x, L) (u32)(OPCODE_X10(c, x) | (L) << 21)
#define OPCODE_MTCRF(c, xo, crf) (u32)(OPCODE_X10(c, xo) | (crf) << 12)

namespace {
    // オペランドのテンプレート
    // [RegTemplateBegin, RegTemplateEnd] は，命令中のオペランドレジスタ番号に置き換えられる
    // [ImmTemplateBegin, RegTemplateEnd] は，即値に置き換えられる

    // レジスタ・テンプレートに使用する番号
    // 本物のレジスタ番号を使ってはならない
    static const int RegTemplateBegin = -20;    // 命令中のレジスタ番号に変換 (数値に意味はない．本物のレジスタ番号と重ならずかつ一意であればよい)
    static const int RegTemplateEnd = RegTemplateBegin+4-1;
    static const int ImmTemplateBegin = -30;    // 即値に変換
    static const int ImmTemplateEnd = ImmTemplateBegin+4-1;

    const int R0 = RegTemplateBegin+0;
    const int R1 = RegTemplateBegin+1;
    const int R2 = RegTemplateBegin+2;
    const int R3 = RegTemplateBegin+3;

    const int I0 = ImmTemplateBegin+0;
    const int I1 = ImmTemplateBegin+1;
    const int I2 = ImmTemplateBegin+2;
    const int I3 = ImmTemplateBegin+3;

    const int CR0 = PPC64Info::REG_CR0;
    const int CR1 = CR0 + 1;
    const int CR2 = CR0 + 2;
    const int CR3 = CR0 + 3;
    const int CR4 = CR0 + 4;
    const int CR5 = CR0 + 5;
    const int CR6 = CR0 + 6;
    const int CR7 = CR0 + 7;

    const int LR = PPC64Info::REG_LINK;
    const int CTR = PPC64Info::REG_COUNT;
    //const int XER = PPC64Info::REG_XER;
    const int FPC = PPC64Info::REG_FPSCR;
    const int CA = PPC64Info::REG_CA;
    const int AR = PPC64Info::REG_ADDRESS;
}

#define PPC64DSTOP(n) DstOperand<n> 
#define PPC64SRCOP(n) SrcOperand<n> 
#define PPC64SRCOPFLOAT(n) Cast< float, AsFP< double, SrcOperand<n> > >
#define PPC64SRCOPDOUBLE(n) AsFP< double, SrcOperand<n> >

#define D0 PPC64DSTOP(0)
#define D1 PPC64DSTOP(1)
#define D2 PPC64DSTOP(2)
#define S0 PPC64SRCOP(0)
#define S1 PPC64SRCOP(1)
#define S2 PPC64SRCOP(2)
#define S3 PPC64SRCOP(3)
#define S4 PPC64SRCOP(4)
#define S5 PPC64SRCOP(5)
#define SF0 PPC64SRCOPFLOAT(0)
#define SF1 PPC64SRCOPFLOAT(1)
#define SF2 PPC64SRCOPFLOAT(2)
#define SF3 PPC64SRCOPFLOAT(3)
#define SF4 PPC64SRCOPFLOAT(4)
#define SF5 PPC64SRCOPFLOAT(5)
#define SD0 PPC64SRCOPDOUBLE(0)
#define SD1 PPC64SRCOPDOUBLE(1)
#define SD2 PPC64SRCOPDOUBLE(2)
#define SD3 PPC64SRCOPDOUBLE(3)
#define SD4 PPC64SRCOPDOUBLE(4)
#define SD5 PPC64SRCOPDOUBLE(5)

// 60000000 ori r0, r0, 0 = nop

// no trap implemented

// 投機的にフェッチされたときにはエラーにせず，実行されたときにエラーにする
// syscallにすることにより，直前までの命令が完了してから実行される (実行は投機的でない)
PPC64Converter::OpDef PPC64Converter::m_OpDefUnknown = 
    {"unknown", MASK_EXACT, 0,  1, {{OpClassCode::UNDEF,    { -1, -1, -1}, {I0, -1, -1, -1, -1, -1},    PPC64Converter::PPC64UnknownOperation}}};


// branchは，OpInfo 列の最後じゃないとだめ
PPC64Converter::OpDef PPC64Converter::m_OpDefsBase[] = 
{
    // system call
    // r0 = id
    // r3-r8 = args
    // r3 = ret value
    // CR0:so = error
    {"sc",  MASK_OP, OPCODE_OP(17), 2, {
        {OpClassCode::syscall,  {-1, -1}, { 0,  3,  4, -1, -1, -1}, PPC64SyscallSetArg},
        {OpClassCode::syscall_branch,   { 3,CR0}, { 5,  6,  7,  8, -1, -1}, PPC64SyscallCore},
    }},

    //
    // special instructions
    //
    {"nop",     MASK_EXACT, 0x60000000, 1, {{OpClassCode::iNOP, { -1, -1, -1}, { -1, -1, -1, -1, -1, -1},   NoOperation}}},
    //{"fnop",  MASK_EXACT, 0xxxxxxxxx, 1, {{OpClassCode::fNOP, { -1, -1, -1}, { -1, -1, -1, -1, -1, -1},   NoOperation}}},
    //{"unop",  MASK_EXACT, 0xxxxxxxxx, 1, {{OpClassCode::iNOP, { -1, -1, -1}, { -1, -1, -1, -1, -1, -1},   NoOperation}}},


    //
    // integer
    //

    // - 次の命令は使用されない？ : td, tdi, tw, twi, isel, div, lmw, smw, string load/store, byte-reversal load/store
    //                              OE=1 のもの

    // arithmetic

    // addi, adds の RA=0 版
    {"li",      MASK_OP | MASK_RA, OPCODE_OP(14), 1, {{OpClassCode::iIMM,  { R0, -1, -1}, { I0, -1, -1, -1, -1, -1}, Set< D0, S0 >}}},
    {"lis",     MASK_OP | MASK_RA, OPCODE_OP(15), 1, {{OpClassCode::iIMM,  { R0, -1, -1}, { I0, -1, -1, -1, -1, -1}, Set< D0, LShiftL< u64, S0, IntConst<unsigned int,16>, 63> >}}},

    {"addi",    MASK_OP, OPCODE_OP(14), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1}, Set< D0, IntAdd< u64, S0, S1 > >}}},
    {"adds",    MASK_OP, OPCODE_OP(15), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1}, Set< D0, IntAdd< u64, S0, LShiftL< u64, S1, IntConst<unsigned int,16>, 63> > >}}},

    {"add",     MASK_X10FOE, OPCODE_X10FOE(31, 266, 0, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1},       Set< D0,     IntAdd< u64, S0, S1 > >}}},
    {"add.",    MASK_X10FOE, OPCODE_X10FOE(31, 266, 0, 1), 1, {{OpClassCode::iALU,  { R0,CR0, -1}, { R1, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, IntAdd< u64, S0, S1 > >}}},
    
    {"subf",    MASK_X10FOE, OPCODE_X10FOE(31,  40, 0, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1},       Set< D0,     IntSub< u64, S1, S0 > >}}},
    {"subf.",   MASK_X10FOE, OPCODE_X10FOE(31,  40, 0, 1), 1, {{OpClassCode::iALU,  { R0,CR0, -1}, { R1, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, IntSub< u64, S1, S0 > >}}},

    {"addic",   MASK_OP, OPCODE_OP(12), 1, {{OpClassCode::iALU,  { R0, CA, -1}, { R1, I0, -1, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     CarryOfAdd< u64, S0, S1 > >,
                  Set< D0,     IntAdd< u64, S0, S1 > >
        >
    }}},
    {"addic.",  MASK_OP, OPCODE_OP(13), 1, {{OpClassCode::iALU,  { R0, CA,CR0}, { R1, I0, -1, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     CarryOfAdd< u64, S0, S1 > >,
            PPC64SetF< D0, D2, IntAdd< u64, S0, S1 > >
        >
    }}},

    // Carry Flag は Borrow Flag の否定として使用される
    {"subfic",  MASK_OP, OPCODE_OP(8), 1, {{OpClassCode::iALU,  { R0, CA, -1}, { R1, I0, -1, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     BitXor< u64, BorrowOfSub< u64, S1, S0>, IntConst<u64, (u64)1> > >,
                  Set< D0,     IntSub< u64, S1, S0 > >
        >
    }}},

    {"addc",    MASK_X10FOE, OPCODE_X10FOE(31,  10, 0, 0), 1, {{OpClassCode::iALU,  { R0, CA, -1}, { R1, R2, -1, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     CarryOfAdd< u64, S0, S1 > >,
                  Set< D0,     IntAdd< u64, S0, S1 > >
        >
    }}},
    {"addc.",   MASK_X10FOE, OPCODE_X10FOE(31,  10, 0, 1), 1, {{OpClassCode::iALU,  { R0, CA,CR0}, { R1, R2, -1, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     CarryOfAdd< u64, S0, S1 > >,
            PPC64SetF< D0, D2, IntAdd< u64, S0, S1 > >
        >
    }}},
    {"subfc",   MASK_X10FOE, OPCODE_X10FOE(31,   8, 0, 0), 1, {{OpClassCode::iALU,  { R0, CA, -1}, { R1, R2, -1, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     BitXor< u64, BorrowOfSub< u64, S1, S0>, IntConst<u64, (u64)1> > >,
                  Set< D0,     IntSub< u64, S1, S0 > >
        >
    }}},
    {"subfc.",  MASK_X10FOE, OPCODE_X10FOE(31,   8, 0, 1), 1, {{OpClassCode::iALU,  { R0, CA,CR0}, { R1, R2, -1, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     BitXor< u64, BorrowOfSub< u64, S1, S0>, IntConst<u64, (u64)1> > >,
            PPC64SetF< D0, D2, IntSub< u64, S1, S0 > >
        >
    }}},

    {"adde",    MASK_X10FOE, OPCODE_X10FOE(31, 138, 0, 0), 1, {{OpClassCode::iALU,  { R0, CA, -1}, { R1, R2, CA, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     CarryOfAddWithCarry< u64, S0, S1, S2 > >,
                  Set< D0,     IntAdd< u64, IntAdd< u64, S0, S1 >, S2> >
        >
    }}},
    {"adde.",   MASK_X10FOE, OPCODE_X10FOE(31, 138, 0, 1), 1, {{OpClassCode::iALU,  { R0, CA,CR0}, { R1, R2, CA, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     CarryOfAddWithCarry< u64, S0, S1, S2 > >,
            PPC64SetF< D0, D2, IntAdd< u64, IntAdd< u64, S0, S1 >, S2> >
        >
    }}},
    // BitXor< u64, S2, IntConst<u64, (u64)1> > = BO = !CA
    {"subfe",   MASK_X10FOE, OPCODE_X10FOE(31, 136, 0, 0), 1, {{OpClassCode::iALU,  { R0, CA, -1}, { R1, R2, CA, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     BitXor< u64, BorrowOfSubWithBorrow< u64, S1, S0, BitXor< u64, S2, IntConst<u64, (u64)1> > >, IntConst<u64, (u64)1> > >,
                  Set< D0,     IntSub< u64, S1, IntAdd< u64, S0, BitXor< u64, S2, IntConst<u64, (u64)1> > > > >     // lhs - (rhs+BO)
        >
    }}},
    {"subfe.",  MASK_X10FOE, OPCODE_X10FOE(31, 136, 0, 1), 1, {{OpClassCode::iALU,  { R0, CA,CR0}, { R1, R2, CA, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     BitXor< u64, BorrowOfSubWithBorrow< u64, S1, S0, BitXor< u64, S2, IntConst<u64, (u64)1> > >, IntConst<u64, (u64)1> > >,
            PPC64SetF< D0, D2, IntSub< u64, S1, IntAdd< u64, S0, BitXor< u64, S2, IntConst<u64, (u64)1> > > > >     // lhs - (rhs+BO)
        >
    }}},

    {"addme",   MASK_X10FOE, OPCODE_X10FOE(31, 234, 0, 0), 1, {{OpClassCode::iALU,  { R0, CA, -1}, { R1, CA, -1, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     CarryOfAddWithCarry< u64, S0, IntConst<u64, ~(u64)0>, S1 > >,
                  Set< D0,     IntAdd< u64, IntAdd< u64, S0, IntConst<u64, ~(u64)0> >, S1> >
        >
    }}},
    {"addme.",  MASK_X10FOE, OPCODE_X10FOE(31, 234, 0, 1), 1, {{OpClassCode::iALU,  { R0, CA,CR0}, { R1, CA, -1, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     CarryOfAddWithCarry< u64, S0, IntConst<u64, ~(u64)0>, S1 > >,
            PPC64SetF< D0, D2, IntAdd< u64, IntAdd< u64, S0, IntConst<u64, ~(u64)0> >, S1> >
        >
    }}},
    {"subfme",  MASK_X10FOE, OPCODE_X10FOE(31, 232, 0, 0), 1, {{OpClassCode::iALU,  { R0, CA, -1}, { R1, CA, -1, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     BitXor< u64, BorrowOfSubWithBorrow< u64, IntConst<u64, ~(u64)0>, S0, BitXor< u64, S1, IntConst<u64, (u64)1> > >, IntConst<u64, (u64)1> > >,
                  Set< D0,     IntSub< u64, IntConst<u64, ~(u64)0>, IntAdd< u64, S0, BitXor< u64, S1, IntConst<u64, (u64)1> > > > >     // lhs - (rhs+BO)
        >
    }}},
    {"subfme.", MASK_X10FOE, OPCODE_X10FOE(31, 232, 0, 1), 1, {{OpClassCode::iALU,  { R0, CA,CR0}, { R1, CA, -1, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     BitXor< u64, BorrowOfSubWithBorrow< u64, IntConst<u64, ~(u64)0>, S0, BitXor< u64, S1, IntConst<u64, (u64)1> > >, IntConst<u64, (u64)1> > >,
            PPC64SetF< D0, D2, IntSub< u64, IntConst<u64, ~(u64)0>, IntAdd< u64, S0, BitXor< u64, S1, IntConst<u64, (u64)1> > > > >     // lhs - (rhs+BO)
        >
    }}},

    {"addze",   MASK_X10FOE, OPCODE_X10FOE(31, 202, 0, 0), 1, {{OpClassCode::iALU,  { R0, CA, -1}, { R1, CA, -1, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     CarryOfAdd< u64, S0, S1 > >,
                  Set< D0,     IntAdd< u64, S0, S1 > >
        >
    }}},
    {"addze.",  MASK_X10FOE, OPCODE_X10FOE(31, 202, 0, 1), 1, {{OpClassCode::iALU,  { R0, CA,CR0}, { R1, CA, -1, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     CarryOfAdd< u64, S0, S1 > >,
            PPC64SetF< D0, D2, IntAdd< u64, S0, S1 > >
        >
    }}},
    {"subfze",  MASK_X10FOE, OPCODE_X10FOE(31, 200, 0, 0), 1, {{OpClassCode::iALU,  { R0, CA, -1}, { R1, CA, -1, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     BitXor< u64, BorrowOfSubWithBorrow< u64, IntConst<u64, 0>, S0, BitXor< u64, S1, IntConst<u64, (u64)1> > >, IntConst<u64, (u64)1> > >,
                  Set< D0,     IntSub< u64, IntConst<u64, 0>, IntAdd< u64, S0, BitXor< u64, S1, IntConst<u64, (u64)1> > > > >       // lhs - (rhs+BO)
        >
    }}},
    {"subfze.", MASK_X10FOE, OPCODE_X10FOE(31, 200, 0, 1), 1, {{OpClassCode::iALU,  { R0, CA,CR0}, { R1, CA, -1, -1, -1, -1}, 
        Sequence2<
                  Set< D1,     BitXor< u64, BorrowOfSubWithBorrow< u64, IntConst<u64, 0>, S0, BitXor< u64, S1, IntConst<u64, (u64)1> > >, IntConst<u64, (u64)1> > >,
            PPC64SetF< D0, D2, IntSub< u64, IntConst<u64, 0>, IntAdd< u64, S0, BitXor< u64, S1, IntConst<u64, (u64)1> > > > >       // lhs - (rhs+BO)
        >
    }}},

    {"neg",     MASK_X10FOE, OPCODE_X10FOE(31, 104, 0, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, -1, -1, -1, -1, -1},       Set< D0,     IntNeg<u64, S0> >}}},
    {"neg.",    MASK_X10FOE, OPCODE_X10FOE(31, 104, 0, 1), 1, {{OpClassCode::iALU,  { R0,CR0, -1}, { R1, -1, -1, -1, -1, -1}, PPC64SetF< D0, D1, IntNeg<u64, S0> >}}},

    {"mulli",   MASK_OP, OPCODE_OP(7), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1},       Set< D0,     IntMul< s64, S0, S1 > >}}},

    {"mullw",   MASK_X10FOE, OPCODE_X10FOE(31, 235, 0, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1},       Set< D0,     IntMul< s64, Cast<s32, S0>, Cast<s32, S1> > >}}},
    {"mullw.",  MASK_X10FOE, OPCODE_X10FOE(31, 235, 0, 1), 1, {{OpClassCode::iALU,  { R0,CR0, -1}, { R1, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, IntMul< s64, Cast<s32, S0>, Cast<s32, S1> > >}}},

    // AShiftR: 仕様では上位32ビットはundefinedだが，実機 (PS3) では符号拡張してるようなので
    {"mulhw",   MASK_X10FOE, OPCODE_X10FOE(31,  75, 0, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1},       Set< D0,     AShiftR< s64, IntMul< s64, Cast<s32, S0>, Cast<s32, S1> >, IntConst<unsigned int, 32>, 63 > >}}},
    {"mulhw.",  MASK_X10FOE, OPCODE_X10FOE(31,  75, 0, 1), 1, {{OpClassCode::iALU,  { R0,CR0, -1}, { R1, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, AShiftR< s64, IntMul< s64, Cast<s32, S0>, Cast<s32, S1> >, IntConst<unsigned int, 32>, 63 > >}}},

    {"mulhwu",  MASK_X10FOE, OPCODE_X10FOE(31,  11, 0, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1},       Set< D0,     LShiftR< u64, IntMul< u64, Cast<u32, S0>, Cast<u32, S1> >, IntConst<unsigned int, 32>, 63 > >}}},
    {"mulhwu.", MASK_X10FOE, OPCODE_X10FOE(31,  11, 0, 1), 1, {{OpClassCode::iALU,  { R0,CR0, -1}, { R1, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, LShiftR< u64, IntMul< u64, Cast<u32, S0>, Cast<u32, S1> >, IntConst<unsigned int, 32>, 63 > >}}},

    {"divw",    MASK_X10FOE, OPCODE_X10FOE(31, 491, 0, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1},       Set< D0,     IntDiv< s32, S0, S1 > >}}},
    {"divw.",   MASK_X10FOE, OPCODE_X10FOE(31, 491, 0, 1), 1, {{OpClassCode::iALU,  { R0,CR0, -1}, { R1, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, IntDiv< s32, S0, S1 > >}}},

    {"divwu",   MASK_X10FOE, OPCODE_X10FOE(31, 459, 0, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1},       Set< D0,     IntDiv< u32, S0, S1 > >}}},
    {"divwu.",  MASK_X10FOE, OPCODE_X10FOE(31, 459, 0, 1), 1, {{OpClassCode::iALU,  { R0,CR0, -1}, { R1, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, IntDiv< u32, S0, S1 > >}}},

    {"mulld",   MASK_X10FOE, OPCODE_X10FOE(31, 233, 0, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1},       Set< D0,     IntMul< s64, Cast<s64, S0>, Cast<s64, S1> > >}}},
    {"mulld.",  MASK_X10FOE, OPCODE_X10FOE(31, 233, 0, 1), 1, {{OpClassCode::iALU,  { R0,CR0, -1}, { R1, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, IntMul< s64, Cast<s64, S0>, Cast<s64, S1> > >}}},

    {"mulhd",   MASK_X10FOE, OPCODE_X10FOE(31,  73, 0, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1},       Set< D0,     IntSMulh64< S0, S1> >}}},
    {"mulhd.",  MASK_X10FOE, OPCODE_X10FOE(31,  73, 0, 1), 1, {{OpClassCode::iALU,  { R0,CR0, -1}, { R1, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, IntSMulh64< S0, S1> >}}},

    {"mulhdu",  MASK_X10FOE, OPCODE_X10FOE(31,   9, 0, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1},       Set< D0,     IntUMulh64< S0, S1> >}}},
    {"mulhdu.", MASK_X10FOE, OPCODE_X10FOE(31,   9, 0, 1), 1, {{OpClassCode::iALU,  { R0,CR0, -1}, { R1, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, IntUMulh64< S0, S1> >}}},

    {"divd",    MASK_X10FOE, OPCODE_X10FOE(31, 489, 0, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1},       Set< D0,     IntDiv< s64, S0, S1> >}}},
    {"divd.",   MASK_X10FOE, OPCODE_X10FOE(31, 489, 0, 1), 1, {{OpClassCode::iALU,  { R0,CR0, -1}, { R1, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, IntDiv< s64, S0, S1> >}}},

    {"divdu",   MASK_X10FOE, OPCODE_X10FOE(31, 457, 0, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1},       Set< D0,     IntDiv< u64, S0, S1> >}}},
    {"divdu.",  MASK_X10FOE, OPCODE_X10FOE(31, 457, 0, 1), 1, {{OpClassCode::iALU,  { R0,CR0, -1}, { R1, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, IntDiv< u64, S0, S1> >}}},

    // compare
    {"cmpwi",   MASK_DCMP, OPCODE_DCMP(11, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1}, Set< D0, PPC64Compare< s32, S0, S1 > >}}},
    {"cmpdi",   MASK_DCMP, OPCODE_DCMP(11, 1), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1}, Set< D0, PPC64Compare< s64, S0, S1 > >}}},
    {"cmpw",    MASK_X10CMP, OPCODE_X10CMP(31,   0, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1}, Set< D0, PPC64Compare< s32, S0, S1 > >}}},
    {"cmpd",    MASK_X10CMP, OPCODE_X10CMP(31,   0, 1), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1}, Set< D0, PPC64Compare< s64, S0, S1 > >}}},

    {"cmplwi",  MASK_DCMP, OPCODE_DCMP(10, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1}, Set< D0, PPC64Compare< u32, S0, S1 > >}}},
    {"cmpldi",  MASK_DCMP, OPCODE_DCMP(10, 1), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1}, Set< D0, PPC64Compare< u64, S0, S1 > >}}},
    {"cmplw",   MASK_X10CMP, OPCODE_X10CMP(31,  32, 0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1}, Set< D0, PPC64Compare< u32, S0, S1 > >}}},
    {"cmpld",   MASK_X10CMP, OPCODE_X10CMP(31,  32, 1), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1}, Set< D0, PPC64Compare< u64, S0, S1 > >}}},

    // logical
    {"andi.",   MASK_OP, OPCODE_OP(28), 1, {{OpClassCode::iALU,  { R1,CR0, -1}, { R0, I0, -1, -1, -1, -1}, PPC64SetF< D0, D1, BitAnd< u64, S0, S1 > >}}},
    {"andis.",  MASK_OP, OPCODE_OP(29), 1, {{OpClassCode::iALU,  { R1,CR0, -1}, { R0, I0, -1, -1, -1, -1}, PPC64SetF< D0, D1, BitAnd< u64, S0, LShiftL< u64, S1, IntConst<unsigned int,16>, 63> > >}}},
    {"ori",     MASK_OP, OPCODE_OP(24), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, I0, -1, -1, -1, -1}, Set< D0, BitOr< u64, S0, S1 > >}}},
    {"oris",    MASK_OP, OPCODE_OP(25), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, I0, -1, -1, -1, -1}, Set< D0, BitOr< u64, S0, LShiftL< u64, S1, IntConst<unsigned int,16>, 63> > >}}},
    {"xori",    MASK_OP, OPCODE_OP(26), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, I0, -1, -1, -1, -1}, Set< D0, BitXor< u64, S0, S1 > >}}},
    {"xoris",   MASK_OP, OPCODE_OP(27), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, I0, -1, -1, -1, -1}, Set< D0, BitXor< u64, S0, LShiftL< u64, S1, IntConst<unsigned int,16>, 63> > >}}},

    {"and",     MASK_X10F, OPCODE_X10F(31,  28, 0), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, R2, -1, -1, -1, -1},       Set< D0,     BitAnd< u64, S0, S1 > >}}},
    {"and.",    MASK_X10F, OPCODE_X10F(31,  28, 1), 1, {{OpClassCode::iALU,  { R1,CR0, -1}, { R0, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, BitAnd< u64, S0, S1 > >}}},
    {"or",      MASK_X10F, OPCODE_X10F(31, 444, 0), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, R2, -1, -1, -1, -1},       Set< D0,     BitOr< u64, S0, S1 > >}}},
    {"or.",     MASK_X10F, OPCODE_X10F(31, 444, 1), 1, {{OpClassCode::iALU,  { R1,CR0, -1}, { R0, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, BitOr< u64, S0, S1 > >}}},
    {"xor",     MASK_X10F, OPCODE_X10F(31, 316, 0), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, R2, -1, -1, -1, -1},       Set< D0,     BitXor< u64, S0, S1 > >}}},
    {"xor.",    MASK_X10F, OPCODE_X10F(31, 316, 1), 1, {{OpClassCode::iALU,  { R1,CR0, -1}, { R0, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, BitXor< u64, S0, S1 > >}}},
    {"nand",    MASK_X10F, OPCODE_X10F(31, 476, 0), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, R2, -1, -1, -1, -1},       Set< D0,     BitNand< u64, S0, S1 > >}}},
    {"nand.",   MASK_X10F, OPCODE_X10F(31, 476, 1), 1, {{OpClassCode::iALU,  { R1,CR0, -1}, { R0, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, BitNand< u64, S0, S1 > >}}},
    {"nor",     MASK_X10F, OPCODE_X10F(31, 124, 0), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, R2, -1, -1, -1, -1},       Set< D0,     BitNor< u64, S0, S1 > >}}},
    {"nor.",    MASK_X10F, OPCODE_X10F(31, 124, 1), 1, {{OpClassCode::iALU,  { R1,CR0, -1}, { R0, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, BitNor< u64, S0, S1 > >}}},
    {"eqv",     MASK_X10F, OPCODE_X10F(31, 284, 0), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, R2, -1, -1, -1, -1},       Set< D0,     BitXorNot< u64, S0, S1 > >}}},
    {"eqv.",    MASK_X10F, OPCODE_X10F(31, 284, 1), 1, {{OpClassCode::iALU,  { R1,CR0, -1}, { R0, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, BitXorNot< u64, S0, S1 > >}}},
    {"andc",    MASK_X10F, OPCODE_X10F(31,  60, 0), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, R2, -1, -1, -1, -1},       Set< D0,     BitAndNot< u64, S0, S1 > >}}},
    {"andc.",   MASK_X10F, OPCODE_X10F(31,  60, 1), 1, {{OpClassCode::iALU,  { R1,CR0, -1}, { R0, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, BitAndNot< u64, S0, S1 > >}}},
    {"orc",     MASK_X10F, OPCODE_X10F(31, 412, 0), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, R2, -1, -1, -1, -1},       Set< D0,     BitOrNot< u64, S0, S1 > >}}},
    {"orc.",    MASK_X10F, OPCODE_X10F(31, 412, 1), 1, {{OpClassCode::iALU,  { R1,CR0, -1}, { R0, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, BitOrNot< u64, S0, S1 > >}}},
    
    {"extsb",   MASK_X10F, OPCODE_X10F(31, 954, 0), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, -1, -1, -1, -1, -1},       SetSext< D0,     Cast< u8, S0 > >}}},
    {"extsb.",  MASK_X10F, OPCODE_X10F(31, 954, 1), 1, {{OpClassCode::iALU,  { R1,CR0, -1}, { R0, -1, -1, -1, -1, -1}, PPC64SetSextF< D0, D1, Cast< u8, S0 > >}}},
    {"extsh",   MASK_X10F, OPCODE_X10F(31, 922, 0), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, -1, -1, -1, -1, -1},       SetSext< D0,     Cast< u16, S0 > >}}},
    {"extsh.",  MASK_X10F, OPCODE_X10F(31, 922, 1), 1, {{OpClassCode::iALU,  { R1,CR0, -1}, { R0, -1, -1, -1, -1, -1}, PPC64SetSextF< D0, D1, Cast< u16, S0 > >}}},
    {"extsw",   MASK_X10F, OPCODE_X10F(31, 986, 0), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, -1, -1, -1, -1, -1},       SetSext< D0,     Cast< u32, S0 > >}}},
    {"extsw.",  MASK_X10F, OPCODE_X10F(31, 986, 1), 1, {{OpClassCode::iALU,  { R1,CR0, -1}, { R0, -1, -1, -1, -1, -1}, PPC64SetSextF< D0, D1, Cast< u32, S0 > >}}},
    
    {"cntlzw",  MASK_X10F, OPCODE_X10F(31,  26, 0), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, -1, -1, -1, -1, -1},       Set< D0,     PPC64Cntlz< u32, S0 > >}}},
    {"cntlzw.", MASK_X10F, OPCODE_X10F(31,  26, 1), 1, {{OpClassCode::iALU,  { R1,CR0, -1}, { R0, -1, -1, -1, -1, -1}, PPC64SetF< D0, D1, PPC64Cntlz< u32, S0 > >}}},
    {"cntlzd",  MASK_X10F, OPCODE_X10F(31,  58, 0), 1, {{OpClassCode::iALU,  { R1, -1, -1}, { R0, -1, -1, -1, -1, -1},       Set< D0,     PPC64Cntlz< u64, S0 > >}}},
    {"cntlzd.", MASK_X10F, OPCODE_X10F(31,  58, 1), 1, {{OpClassCode::iALU,  { R1,CR0, -1}, { R0, -1, -1, -1, -1, -1}, PPC64SetF< D0, D1, PPC64Cntlz< u64, S0 > >}}},

    // rotate
    {"rlwinm",  MASK_OPF, OPCODE_OPF(21, 0), 1, {{OpClassCode::iSFT,  { R1, -1, -1}, { R0, I0, I1, I2, -1, -1},       Set< D0,     PPC64Mask< u32, RotateL<u32, S0, S1>, S2, S3 > >}}},
    {"rlwinm.", MASK_OPF, OPCODE_OPF(21, 1), 1, {{OpClassCode::iSFT,  { R1,CR0, -1}, { R0, I0, I1, I2, -1, -1}, PPC64SetF< D0, D1, PPC64Mask< u32, RotateL<u32, S0, S1>,  S2, S3 > >}}},
    {"rlwnm",   MASK_OPF, OPCODE_OPF(23, 0), 1, {{OpClassCode::iSFT,  { R1, -1, -1}, { R0, R2, I0, I1, -1, -1},       Set< D0,     PPC64Mask< u32, RotateL<u32, S0, S1>, S2, S3 > >}}},
    {"rlwnm.",  MASK_OPF, OPCODE_OPF(23, 1), 1, {{OpClassCode::iSFT,  { R1,CR0, -1}, { R0, R2, I0, I1, -1, -1}, PPC64SetF< D0, D1, PPC64Mask< u32, RotateL<u32, S0, S1>, S2, S3 > >}}},
    {"rlwimi",  MASK_OPF, OPCODE_OPF(20, 0), 1, {{OpClassCode::iSFT,  { R1, -1, -1}, { R0, R1, I0, I1, I2, -1},       Set< D0,     PPC64MaskInsert< u32, S1, RotateL<u32, S0, S2>, S3, S4 > >}}},
    {"rlwimi.", MASK_OPF, OPCODE_OPF(20, 1), 1, {{OpClassCode::iSFT,  { R1,CR0, -1}, { R0, R1, I0, I1, I2, -1}, PPC64SetF< D0, D1, PPC64MaskInsert< u32, S1, RotateL<u32, S0, S2>, S3, S4 > >}}},

    {"rldicl",  MASK_MD, OPCODE_MD(30, 0, 0), 1, {{OpClassCode::iSFT,  { R1, -1, -1}, { R0, I0, I1, -1, -1, -1},       Set< D0,     PPC64Mask< u64, RotateL<u64, S0, S1>, S2, IntConst<unsigned int, 63> > >}}},
    {"rldicl.", MASK_MD, OPCODE_MD(30, 0, 1), 1, {{OpClassCode::iSFT,  { R1,CR0, -1}, { R0, I0, I1, -1, -1, -1}, PPC64SetF< D0, D1, PPC64Mask< u64, RotateL<u64, S0, S1>, S2, IntConst<unsigned int, 63> > >}}},
    {"rldicr",  MASK_MD, OPCODE_MD(30, 1, 0), 1, {{OpClassCode::iSFT,  { R1, -1, -1}, { R0, I0, I1, -1, -1, -1},       Set< D0,     PPC64Mask< u64, RotateL<u64, S0, S1>, IntConst<unsigned int, 0>, S2 > >}}},
    {"rldicr.", MASK_MD, OPCODE_MD(30, 1, 1), 1, {{OpClassCode::iSFT,  { R1,CR0, -1}, { R0, I0, I1, -1, -1, -1}, PPC64SetF< D0, D1, PPC64Mask< u64, RotateL<u64, S0, S1>, IntConst<unsigned int, 0>, S2 > >}}},
    {"rldic",   MASK_MD, OPCODE_MD(30, 2, 0), 1, {{OpClassCode::iSFT,  { R1, -1, -1}, { R0, I0, I1, -1, -1, -1},       Set< D0,     PPC64Mask< u64, RotateL<u64, S0, S1>, S2, BitNot<u64, S1> > >}}},
    {"rldic.",  MASK_MD, OPCODE_MD(30, 2, 1), 1, {{OpClassCode::iSFT,  { R1,CR0, -1}, { R0, I0, I1, -1, -1, -1}, PPC64SetF< D0, D1, PPC64Mask< u64, RotateL<u64, S0, S1>, S2, BitNot<u64, S1> > >}}},

    {"rldimi",  MASK_MD, OPCODE_MD(30, 3, 0), 1, {{OpClassCode::iSFT,  { R1, -1, -1}, { R0, R1, I0, I1, -1, -1},       Set< D0,     PPC64MaskInsert< u64, S1, RotateL<u64, S0, S2>, S3, BitNot<u64, S2> > >}}},
    {"rldimi.", MASK_MD, OPCODE_MD(30, 3, 1), 1, {{OpClassCode::iSFT,  { R1,CR0, -1}, { R0, R1, I0, I1, -1, -1}, PPC64SetF< D0, D1, PPC64MaskInsert< u64, S1, RotateL<u64, S0, S2>, S3, BitNot<u64, S2> > >}}},

    {"rldcl",   MASK_MDS, OPCODE_MDS(30, 8, 0), 1, {{OpClassCode::iSFT,  { R1, -1, -1}, { R0, R1, I0, -1, -1, -1},       Set< D0,     PPC64Mask< u64, RotateL<u64, S0, S1>, S2, IntConst<unsigned int, 63> > >}}},
    {"rldcl.",  MASK_MDS, OPCODE_MDS(30, 8, 1), 1, {{OpClassCode::iSFT,  { R1,CR0, -1}, { R0, R1, I0, -1, -1, -1}, PPC64SetF< D0, D1, PPC64Mask< u64, RotateL<u64, S0, S1>, S2, IntConst<unsigned int, 63> > >}}},
    {"rldcr",   MASK_MDS, OPCODE_MDS(30, 9, 0), 1, {{OpClassCode::iSFT,  { R1, -1, -1}, { R0, R1, I0, -1, -1, -1},       Set< D0,     PPC64Mask< u64, RotateL<u64, S0, S1>, IntConst<unsigned int, 0>, S2 > >}}},
    {"rldcr.",  MASK_MDS, OPCODE_MDS(30, 9, 1), 1, {{OpClassCode::iSFT,  { R1,CR0, -1}, { R0, R1, I0, -1, -1, -1}, PPC64SetF< D0, D1, PPC64Mask< u64, RotateL<u64, S0, S1>, IntConst<unsigned int, 0>, S2 > >}}},

    // shift
    {"slw",     MASK_X10F, OPCODE_X10F(31,  24, 0), 1, {{OpClassCode::iSFT,  { R1, -1, -1}, { R0, R2, -1, -1, -1, -1},       Set< D0,     LShiftL<u32, S0, S1, 0x3f> >}}},
    {"slw.",    MASK_X10F, OPCODE_X10F(31,  24, 1), 1, {{OpClassCode::iSFT,  { R1,CR0, -1}, { R0, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, LShiftL<u32, S0, S1, 0x3f> >}}},
    {"srw",     MASK_X10F, OPCODE_X10F(31, 536, 0), 1, {{OpClassCode::iSFT,  { R1, -1, -1}, { R0, R2, -1, -1, -1, -1},       Set< D0,     LShiftR<u32, S0, S1, 0x3f> >}}},
    {"srw.",    MASK_X10F, OPCODE_X10F(31, 536, 1), 1, {{OpClassCode::iSFT,  { R1,CR0, -1}, { R0, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, LShiftR<u32, S0, S1, 0x3f> >}}},

    {"srawi",   MASK_X10F, OPCODE_X10F(31, 824, 0), 1, {{OpClassCode::iSFT,  { R1, CA, -1}, { R0, I0, -1, -1, -1, -1},
        Sequence2<
                      Set< D1,     PPC64CarryOfAShiftR<u32, S0, S1, 0x3f> >,
                  SetSext< D0,     AShiftR<u32, S0, S1, 0x3f> >
        >
    }}},
    {"srawi.",  MASK_X10F, OPCODE_X10F(31, 824, 1), 1, {{OpClassCode::iSFT,  { R1, CA,CR0}, { R0, I0, -1, -1, -1, -1},
        Sequence2<
                      Set< D1,     PPC64CarryOfAShiftR<u32, S0, S1, 0x3f> >,
            PPC64SetSextF< D0, D2, AShiftR<u32, S0, S1, 0x3f> >
        >
    }}},
    {"sraw",    MASK_X10F, OPCODE_X10F(31, 792, 0), 1, {{OpClassCode::iSFT,  { R1, CA, -1}, { R0, R2, -1, -1, -1, -1},
        Sequence2<
                      Set< D1,     PPC64CarryOfAShiftR<u32, S0, S1, 0x3f> >,
                  SetSext< D0,     AShiftR<u32, S0, S1, 0x3f> >
        >
    }}},
    {"sraw.",   MASK_X10F, OPCODE_X10F(31, 792, 1), 1, {{OpClassCode::iSFT,  { R1, CA,CR0}, { R0, R2, -1, -1, -1, -1},
        Sequence2<
                      Set< D1,     PPC64CarryOfAShiftR<u32, S0, S1, 0x3f> >,
            PPC64SetSextF< D0, D2, AShiftR<u32, S0, S1, 0x3f> >
        >
    }}},
    {"sld",     MASK_X10F, OPCODE_X10F(31,  27, 0), 1, {{OpClassCode::iSFT,  { R1, -1, -1}, { R0, R2, -1, -1, -1, -1},       Set< D0,     LShiftL<u64, S0, S1, 0x7f> >}}},
    {"sld.",    MASK_X10F, OPCODE_X10F(31,  27, 1), 1, {{OpClassCode::iSFT,  { R1,CR0, -1}, { R0, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, LShiftL<u64, S0, S1, 0x7f> >}}},
    {"srd",     MASK_X10F, OPCODE_X10F(31, 539, 0), 1, {{OpClassCode::iSFT,  { R1, -1, -1}, { R0, R2, -1, -1, -1, -1},       Set< D0,     LShiftR<u64, S0, S1, 0x7f> >}}},
    {"srd.",    MASK_X10F, OPCODE_X10F(31, 539, 1), 1, {{OpClassCode::iSFT,  { R1,CR0, -1}, { R0, R2, -1, -1, -1, -1}, PPC64SetF< D0, D1, LShiftR<u64, S0, S1, 0x7f> >}}},

    {"sradi",   MASK_X10FSH, OPCODE_X10F(31, 826, 0), 1, {{OpClassCode::iSFT,  { R1, CA, -1}, { R0, I0, -1, -1, -1, -1},
        Sequence2<
                  Set< D1,     PPC64CarryOfAShiftR<u64, S0, S1, 0x7f> >,
                  Set< D0,     AShiftR<u64, S0, S1, 0x7f> >
        >
    }}},
    {"sradi.",  MASK_X10FSH, OPCODE_X10F(31, 826, 1), 1, {{OpClassCode::iSFT,  { R1, CA,CR0}, { R0, I0, -1, -1, -1, -1},
        Sequence2<
                  Set< D1,     PPC64CarryOfAShiftR<u64, S0, S1, 0x7f> >,
            PPC64SetF< D0, D2, AShiftR<u64, S0, S1, 0x7f> >
        >
    }}},
    {"srad",    MASK_X10F, OPCODE_X10F(31, 794, 0), 1, {{OpClassCode::iSFT,  { R1, CA, -1}, { R0, R2, -1, -1, -1, -1},
        Sequence2<
                  Set< D1,     PPC64CarryOfAShiftR<u64, S0, S1, 0x7f> >,
                  Set< D0,     AShiftR<u64, S0, S1, 0x7f> >
        >
    }}},
    {"srad.",   MASK_X10F, OPCODE_X10F(31, 794, 1), 1, {{OpClassCode::iSFT,  { R1, CA,CR0}, { R0, R2, -1, -1, -1, -1},
        Sequence2<
                  Set< D1,     PPC64CarryOfAShiftR<u64, S0, S1, 0x7f> >,
            PPC64SetF< D0, D2, AShiftR<u64, S0, S1, 0x7f> >
        >
    }}},


    // mtspr/mfspr

    {"mtlr",    MASK_MTFSPR, OPCODE_MTFSPR(31, 467, 8), 1, {{OpClassCode::iALU,  { LR, -1, -1}, { R0, -1, -1, -1 -1, -1}, Set< D0, S0 >}}},
    {"mtctr",   MASK_MTFSPR, OPCODE_MTFSPR(31, 467, 9), 1, {{OpClassCode::iALU,  {CTR, -1, -1}, { R0, -1, -1, -1 -1, -1}, Set< D0, S0 >}}},
    {"mflr",    MASK_MTFSPR, OPCODE_MTFSPR(31, 339, 8), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { LR, -1, -1, -1 -1, -1}, Set< D0, S0 >}}},
    {"mfctr",   MASK_MTFSPR, OPCODE_MTFSPR(31, 339, 9), 1, {{OpClassCode::iALU,  { R0, -1, -1}, {CTR, -1, -1, -1 -1, -1}, Set< D0, S0 >}}},

    // move from time base : 未実装
    {"mftb",    MASK_MTFSPR, OPCODE_MTFSPR(31, 371, 268), 1, {{OpClassCode::iMOV,  { R0, -1, -1}, { -1, -1, -1, -1, -1, -1}, Set< D0, IntConst<u64, 0> >}}},
    {"mftbu",   MASK_MTFSPR, OPCODE_MTFSPR(31, 371, 269), 1, {{OpClassCode::iMOV,  { R0, -1, -1}, { -1, -1, -1, -1, -1, -1}, Set< D0, IntConst<u64, 0> >}}},

    // condition register

    // とりあえず，mtcrf は1つのCRまたは全体への書き込みのみサポート
    {"mtcrf",   MASK_MTCRF, OPCODE_MTCRF(31, 144, 0x01), 1, {{OpClassCode::iALU,  {CR7, -1, -1}, { R0, -1, -1, -1 -1, -1}, Set< D0, BitExtract< u64, S0, IntConst<unsigned int,  0>, IntConst<unsigned int, 4> > >}}},
    {"mtcrf",   MASK_MTCRF, OPCODE_MTCRF(31, 144, 0x02), 1, {{OpClassCode::iALU,  {CR6, -1, -1}, { R0, -1, -1, -1 -1, -1}, Set< D0, BitExtract< u64, S0, IntConst<unsigned int,  4>, IntConst<unsigned int, 4> > >}}},
    {"mtcrf",   MASK_MTCRF, OPCODE_MTCRF(31, 144, 0x04), 1, {{OpClassCode::iALU,  {CR5, -1, -1}, { R0, -1, -1, -1 -1, -1}, Set< D0, BitExtract< u64, S0, IntConst<unsigned int,  8>, IntConst<unsigned int, 4> > >}}},
    {"mtcrf",   MASK_MTCRF, OPCODE_MTCRF(31, 144, 0x08), 1, {{OpClassCode::iALU,  {CR4, -1, -1}, { R0, -1, -1, -1 -1, -1}, Set< D0, BitExtract< u64, S0, IntConst<unsigned int, 12>, IntConst<unsigned int, 4> > >}}},
    {"mtcrf",   MASK_MTCRF, OPCODE_MTCRF(31, 144, 0x10), 1, {{OpClassCode::iALU,  {CR3, -1, -1}, { R0, -1, -1, -1 -1, -1}, Set< D0, BitExtract< u64, S0, IntConst<unsigned int, 16>, IntConst<unsigned int, 4> > >}}},
    {"mtcrf",   MASK_MTCRF, OPCODE_MTCRF(31, 144, 0x20), 1, {{OpClassCode::iALU,  {CR2, -1, -1}, { R0, -1, -1, -1 -1, -1}, Set< D0, BitExtract< u64, S0, IntConst<unsigned int, 20>, IntConst<unsigned int, 4> > >}}},
    {"mtcrf",   MASK_MTCRF, OPCODE_MTCRF(31, 144, 0x40), 1, {{OpClassCode::iALU,  {CR1, -1, -1}, { R0, -1, -1, -1 -1, -1}, Set< D0, BitExtract< u64, S0, IntConst<unsigned int, 24>, IntConst<unsigned int, 4> > >}}},
    {"mtcrf",   MASK_MTCRF, OPCODE_MTCRF(31, 144, 0x80), 1, {{OpClassCode::iALU,  {CR0, -1, -1}, { R0, -1, -1, -1 -1, -1}, Set< D0, BitExtract< u64, S0, IntConst<unsigned int, 28>, IntConst<unsigned int, 4> > >}}},
    
    {"mtcr",    MASK_MTCRF, OPCODE_MTCRF(31, 144, 0xff), 3, {
        {OpClassCode::iALU,  {CR0,CR1,CR2}, { R0, -1, -1,  -1 -1, -1}, 
            Sequence3<
                Set< D0, BitExtract< u64, S0, IntConst<unsigned int, 28>, IntConst<unsigned int, 4> > >, 
                Set< D1, BitExtract< u64, S0, IntConst<unsigned int, 24>, IntConst<unsigned int, 4> > >, 
                Set< D2, BitExtract< u64, S0, IntConst<unsigned int, 20>, IntConst<unsigned int, 4> > >
            >}, 
        {OpClassCode::iALU,  {CR3,CR4,CR5}, { R0, -1, -1,  -1 -1, -1}, 
            Sequence3<
                Set< D0, BitExtract< u64, S0, IntConst<unsigned int, 16>, IntConst<unsigned int, 4> > >, 
                Set< D1, BitExtract< u64, S0, IntConst<unsigned int, 12>, IntConst<unsigned int, 4> > >, 
                Set< D2, BitExtract< u64, S0, IntConst<unsigned int,  8>, IntConst<unsigned int, 4> > >
            >}, 
        {OpClassCode::iALU,  {CR6,CR7, -1}, { R0, -1, -1,  -1 -1, -1}, 
            Sequence2<
                Set< D0, BitExtract< u64, S0, IntConst<unsigned int,  4>, IntConst<unsigned int, 4> > >, 
                Set< D1, BitExtract< u64, S0, IntConst<unsigned int,  0>, IntConst<unsigned int, 4> > >
            >}, 
    }},

    {"mfcr",    MASK_X10|0x00100000, OPCODE_X10(31,  19), 3, {
        {OpClassCode::iALU,  { R0, -1, -1}, {CR0,CR1,CR2,CR3, -1, -1}, 
            Set< D0, BitOr< u64, BitOr< u64, BitOr< u64, 
                LShiftL<u64, S0, IntConst<unsigned int, 28>, 63 >, 
                LShiftL<u64, S1, IntConst<unsigned int, 24>, 63 > >, 
                LShiftL<u64, S2, IntConst<unsigned int, 20>, 63 > >, 
                LShiftL<u64, S3, IntConst<unsigned int, 16>, 63 > > >, 
        },
        {OpClassCode::iALU,  { R0, -1, -1}, {CR4,CR5, R0, -1, -1, -1}, 
            Set< D0, BitOr< u64, BitOr< u64,
                S2,
                LShiftL<u64, S0, IntConst<unsigned int, 12>, 63 > >, 
                LShiftL<u64, S1, IntConst<unsigned int,  8>, 63 > > >, 
        },
        {OpClassCode::iALU,  { R0, -1, -1}, {CR6,CR7, R0, -1, -1, -1}, 
            Set< D0, BitOr< u64, BitOr< u64, 
                S2,
                LShiftL<u64, S0, IntConst<unsigned int,  4>, 63 > >, 
                LShiftL<u64, S1, IntConst<unsigned int,  0>, 63 > > >, 
        },
    }},




    //
    // FP instructions
    //
    // FX (FPSCR[32]) はサポートしない．FP命令の並列実行ができなくなるため．

    // move
    {"fmr",     MASK_X10F, OPCODE_X10F(63,  72, 0), 1, {{OpClassCode::fMOV,  { R0, -1, -1}, {FPC, R1, -1, -1, -1, -1},         Set< D0,         S1 >}}},
    {"fmr.",    MASK_X10F, OPCODE_X10F(63,  72, 1), 1, {{OpClassCode::fMOV,  { R0,CR1, -1}, {FPC, R1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, SD1 >}}},

    {"fneg",    MASK_X10F, OPCODE_X10F(63,  40, 0), 1, {{OpClassCode::fNEG,  { R0, -1, -1}, {FPC, R1, -1, -1, -1, -1},       SetFP< D0,         FPNeg< double, SD1 > >}}},
    {"fneg.",   MASK_X10F, OPCODE_X10F(63,  40, 1), 1, {{OpClassCode::fNEG,  { R0,CR1, -1}, {FPC, R1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, FPNeg< double, SD1 > >}}},

    {"fabs",    MASK_X10F, OPCODE_X10F(63, 264, 0), 1, {{OpClassCode::fNEG,  { R0, -1, -1}, {FPC, R1, -1, -1, -1, -1},       SetFP< D0,         FPAbs< double, SD1 > >}}},
    {"fabs.",   MASK_X10F, OPCODE_X10F(63, 264, 1), 1, {{OpClassCode::fNEG,  { R0,CR1, -1}, {FPC, R1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, FPAbs< double, SD1 > >}}},

    {"fnabs",   MASK_X10F, OPCODE_X10F(63, 136, 0), 1, {{OpClassCode::fNEG,  { R0, -1, -1}, {FPC, R1, -1, -1, -1, -1},       SetFP< D0,         FPNeg< double, FPAbs<double, SD1> > >}}},
    {"fnabs.",  MASK_X10F, OPCODE_X10F(63, 136, 1), 1, {{OpClassCode::fNEG,  { R0,CR1, -1}, {FPC, R1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, FPNeg< double, FPAbs<double, SD1> > >}}},

    // arithmetic
    {"fadd",    MASK_X5F, OPCODE_X5F(63,  21, 0), 1, {{OpClassCode::fADD,  { R0, -1, -1}, {FPC, R1, R2, -1, -1, -1},       SetFP< D0,                       FPAdd< double, SD1, SD2, PPC64FPSCRRoundMode<S0> > >}}},
    {"fadd.",   MASK_X5F, OPCODE_X5F(63,  21, 1), 1, {{OpClassCode::fADD,  { R0,CR1, -1}, {FPC, R1, R2, -1, -1, -1}, PPC64SetFPF< D0, D1, S0,               FPAdd< double, SD1, SD2, PPC64FPSCRRoundMode<S0> > >}}},
    {"fadds",   MASK_X5F, OPCODE_X5F(59,  21, 0), 1, {{OpClassCode::fADD,  { R0, -1, -1}, {FPC, R1, R2, -1, -1, -1},       SetFP< D0,         Cast< double, FPAdd<  float, SF1, SF2, PPC64FPSCRRoundMode<S0> > > >}}},
    {"fadds.",  MASK_X5F, OPCODE_X5F(59,  21, 1), 1, {{OpClassCode::fADD,  { R0,CR1, -1}, {FPC, R1, R2, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, Cast< double, FPAdd<  float, SF1, SF2, PPC64FPSCRRoundMode<S0> > > >}}},

    {"fsub",    MASK_X5F, OPCODE_X5F(63,  20, 0), 1, {{OpClassCode::fADD,  { R0, -1, -1}, {FPC, R1, R2, -1, -1, -1},       SetFP< D0,                       FPSub< double, SD1, SD2, PPC64FPSCRRoundMode<S0> > >}}},
    {"fsub.",   MASK_X5F, OPCODE_X5F(63,  20, 1), 1, {{OpClassCode::fADD,  { R0,CR1, -1}, {FPC, R1, R2, -1, -1, -1}, PPC64SetFPF< D0, D1, S0,               FPSub< double, SD1, SD2, PPC64FPSCRRoundMode<S0> > >}}},
    {"fsubs",   MASK_X5F, OPCODE_X5F(59,  20, 0), 1, {{OpClassCode::fADD,  { R0, -1, -1}, {FPC, R1, R2, -1, -1, -1},       SetFP< D0,         Cast< double, FPSub<  float, SF1, SF2, PPC64FPSCRRoundMode<S0> > > >}}},
    {"fsubs.",  MASK_X5F, OPCODE_X5F(59,  20, 1), 1, {{OpClassCode::fADD,  { R0,CR1, -1}, {FPC, R1, R2, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, Cast< double, FPSub<  float, SF1, SF2, PPC64FPSCRRoundMode<S0> > > >}}},

    {"fmul",    MASK_X5F, OPCODE_X5F(63,  25, 0), 1, {{OpClassCode::fMUL,  { R0, -1, -1}, {FPC, R1, R2, -1, -1, -1},       SetFP< D0,                       FPMul< double, SD1, SD2, PPC64FPSCRRoundMode<S0> > >}}},
    {"fmul.",   MASK_X5F, OPCODE_X5F(63,  25, 1), 1, {{OpClassCode::fMUL,  { R0,CR1, -1}, {FPC, R1, R2, -1, -1, -1}, PPC64SetFPF< D0, D1, S0,               FPMul< double, SD1, SD2, PPC64FPSCRRoundMode<S0> > >}}},
    {"fmuls",   MASK_X5F, OPCODE_X5F(59,  25, 0), 1, {{OpClassCode::fMUL,  { R0, -1, -1}, {FPC, R1, R2, -1, -1, -1},       SetFP< D0,         Cast< double, FPMul<  float, SF1, SF2, PPC64FPSCRRoundMode<S0> > > >}}},
    {"fmuls.",  MASK_X5F, OPCODE_X5F(59,  25, 1), 1, {{OpClassCode::fMUL,  { R0,CR1, -1}, {FPC, R1, R2, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, Cast< double, FPMul<  float, SF1, SF2, PPC64FPSCRRoundMode<S0> > > >}}},

    {"fdiv",    MASK_X5F, OPCODE_X5F(63,  18, 0), 1, {{OpClassCode::fDIV,  { R0, -1, -1}, {FPC, R1, R2, -1, -1, -1},       SetFP< D0,                       FPDiv< double, SD1, SD2, PPC64FPSCRRoundMode<S0> > >}}},
    {"fdiv.",   MASK_X5F, OPCODE_X5F(63,  18, 1), 1, {{OpClassCode::fDIV,  { R0,CR1, -1}, {FPC, R1, R2, -1, -1, -1}, PPC64SetFPF< D0, D1, S0,               FPDiv< double, SD1, SD2, PPC64FPSCRRoundMode<S0> > >}}},
    {"fdivs",   MASK_X5F, OPCODE_X5F(59,  18, 0), 1, {{OpClassCode::fDIV,  { R0, -1, -1}, {FPC, R1, R2, -1, -1, -1},       SetFP< D0,         Cast< double, FPDiv<  float, SF1, SF2, PPC64FPSCRRoundMode<S0> > > >}}},
    {"fdivs.",  MASK_X5F, OPCODE_X5F(59,  18, 1), 1, {{OpClassCode::fDIV,  { R0,CR1, -1}, {FPC, R1, R2, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, Cast< double, FPDiv<  float, SF1, SF2, PPC64FPSCRRoundMode<S0> > > >}}},

    {"fsqrt",   MASK_X5F, OPCODE_X5F(63,  22, 0), 1, {{OpClassCode::fELEM, { R0, -1, -1}, {FPC, R1, -1, -1, -1, -1},       SetFP< D0,                       FPSqrt< double, SD1, PPC64FPSCRRoundMode<S0> > >}}},
    {"fsqrt.",  MASK_X5F, OPCODE_X5F(63,  22, 1), 1, {{OpClassCode::fELEM, { R0,CR1, -1}, {FPC, R1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0,               FPSqrt< double, SD1, PPC64FPSCRRoundMode<S0> > >}}},
    {"fsqrts",  MASK_X5F, OPCODE_X5F(59,  22, 0), 1, {{OpClassCode::fELEM, { R0, -1, -1}, {FPC, R1, -1, -1, -1, -1},       SetFP< D0,         Cast< double, FPSqrt<  float, SF1, PPC64FPSCRRoundMode<S0> > > >}}},
    {"fsqrts.", MASK_X5F, OPCODE_X5F(59,  22, 1), 1, {{OpClassCode::fELEM, { R0,CR1, -1}, {FPC, R1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, Cast< double, FPSqrt<  float, SF1, PPC64FPSCRRoundMode<S0> > > >}}},

    //fre
    //fsqrtre

    // これらの OpClassCode は？
    {"fmadd",   MASK_X5F, OPCODE_X5F(63,  29, 0), 1, {{OpClassCode::fMUL,  { R0, -1, -1}, {FPC, R1, R2, R3, -1, -1},       SetFP< D0,                       FPAdd< double, FPMul< double, SD1, SD3, PPC64FPSCRRoundMode<S0> >, SD2, PPC64FPSCRRoundMode<S0> > >}}},
    {"fmadd.",  MASK_X5F, OPCODE_X5F(63,  29, 1), 1, {{OpClassCode::fMUL,  { R0,CR1, -1}, {FPC, R1, R2, R3, -1, -1}, PPC64SetFPF< D0, D1, S0,               FPAdd< double, FPMul< double, SD1, SD3, PPC64FPSCRRoundMode<S0> >, SD2, PPC64FPSCRRoundMode<S0> > >}}},
    {"fmadds",  MASK_X5F, OPCODE_X5F(59,  29, 0), 1, {{OpClassCode::fMUL,  { R0, -1, -1}, {FPC, R1, R2, R3, -1, -1},       SetFP< D0,         Cast< double, FPAdd<  float, FPMul<  float, SF1, SF3, PPC64FPSCRRoundMode<S0> >, SF2, PPC64FPSCRRoundMode<S0> > > >}}},
    {"fmadds.", MASK_X5F, OPCODE_X5F(59,  29, 1), 1, {{OpClassCode::fMUL,  { R0,CR1, -1}, {FPC, R1, R2, R3, -1, -1}, PPC64SetFPF< D0, D1, S0, Cast< double, FPAdd<  float, FPMul<  float, SF1, SF3, PPC64FPSCRRoundMode<S0> >, SF2, PPC64FPSCRRoundMode<S0> > > >}}},

    {"fmsub",   MASK_X5F, OPCODE_X5F(63,  28, 0), 1, {{OpClassCode::fMUL,  { R0, -1, -1}, {FPC, R1, R2, R3, -1, -1},       SetFP< D0,                       FPSub< double, FPMul< double, SD1, SD3, PPC64FPSCRRoundMode<S0> >, SD2, PPC64FPSCRRoundMode<S0> > >}}},
    {"fmsub.",  MASK_X5F, OPCODE_X5F(63,  28, 1), 1, {{OpClassCode::fMUL,  { R0,CR1, -1}, {FPC, R1, R2, R3, -1, -1}, PPC64SetFPF< D0, D1, S0,               FPSub< double, FPMul< double, SD1, SD3, PPC64FPSCRRoundMode<S0> >, SD2, PPC64FPSCRRoundMode<S0> > >}}},
    {"fmsubs",  MASK_X5F, OPCODE_X5F(59,  28, 0), 1, {{OpClassCode::fMUL,  { R0, -1, -1}, {FPC, R1, R2, R3, -1, -1},       SetFP< D0,         Cast< double, FPSub<  float, FPMul<  float, SF1, SF3, PPC64FPSCRRoundMode<S0> >, SF2, PPC64FPSCRRoundMode<S0> > > >}}},
    {"fmsubs.", MASK_X5F, OPCODE_X5F(59,  28, 1), 1, {{OpClassCode::fMUL,  { R0,CR1, -1}, {FPC, R1, R2, R3, -1, -1}, PPC64SetFPF< D0, D1, S0, Cast< double, FPSub<  float, FPMul<  float, SF1, SF3, PPC64FPSCRRoundMode<S0> >, SF2, PPC64FPSCRRoundMode<S0> > > >}}},

    {"fnmadd",  MASK_X5F, OPCODE_X5F(63,  31, 0), 1, {{OpClassCode::fMUL,  { R0, -1, -1}, {FPC, R1, R2, R3, -1, -1},       SetFP< D0,                       FPNeg< double, FPAdd< double, FPMul< double, SD1, SD3, PPC64FPSCRRoundMode<S0> >, SD2, PPC64FPSCRRoundMode<S0> > > >}}},
    {"fnmadd.", MASK_X5F, OPCODE_X5F(63,  31, 1), 1, {{OpClassCode::fMUL,  { R0,CR1, -1}, {FPC, R1, R2, R3, -1, -1}, PPC64SetFPF< D0, D1, S0,               FPNeg< double, FPAdd< double, FPMul< double, SD1, SD3, PPC64FPSCRRoundMode<S0> >, SD2, PPC64FPSCRRoundMode<S0> > > >}}},
    {"fnmadds", MASK_X5F, OPCODE_X5F(59,  31, 0), 1, {{OpClassCode::fMUL,  { R0, -1, -1}, {FPC, R1, R2, R3, -1, -1},       SetFP< D0,         Cast< double, FPNeg<  float, FPAdd<  float, FPMul<  float, SF1, SF3, PPC64FPSCRRoundMode<S0> >, SF2, PPC64FPSCRRoundMode<S0> > > > >}}},
    {"fnmadds.",MASK_X5F, OPCODE_X5F(59,  31, 1), 1, {{OpClassCode::fMUL,  { R0,CR1, -1}, {FPC, R1, R2, R3, -1, -1}, PPC64SetFPF< D0, D1, S0, Cast< double, FPNeg<  float, FPAdd<  float, FPMul<  float, SF1, SF3, PPC64FPSCRRoundMode<S0> >, SF2, PPC64FPSCRRoundMode<S0> > > > >}}},

    {"fnmsub",  MASK_X5F, OPCODE_X5F(63,  30, 0), 1, {{OpClassCode::fMUL,  { R0, -1, -1}, {FPC, R1, R2, R3, -1, -1},       SetFP< D0,                       FPNeg< double, FPSub< double, FPMul< double, SD1, SD3, PPC64FPSCRRoundMode<S0> >, SD2, PPC64FPSCRRoundMode<S0> > > >}}},
    {"fnmsub.", MASK_X5F, OPCODE_X5F(63,  30, 1), 1, {{OpClassCode::fMUL,  { R0,CR1, -1}, {FPC, R1, R2, R3, -1, -1}, PPC64SetFPF< D0, D1, S0,               FPNeg< double, FPSub< double, FPMul< double, SD1, SD3, PPC64FPSCRRoundMode<S0> >, SD2, PPC64FPSCRRoundMode<S0> > > >}}},
    {"fnmsubs", MASK_X5F, OPCODE_X5F(59,  30, 0), 1, {{OpClassCode::fMUL,  { R0, -1, -1}, {FPC, R1, R2, R3, -1, -1},       SetFP< D0,         Cast< double, FPNeg<  float, FPSub<  float, FPMul<  float, SF1, SF3, PPC64FPSCRRoundMode<S0> >, SF2, PPC64FPSCRRoundMode<S0> > > > >}}},
    {"fnmsubs.",MASK_X5F, OPCODE_X5F(59,  30, 1), 1, {{OpClassCode::fMUL,  { R0,CR1, -1}, {FPC, R1, R2, R3, -1, -1}, PPC64SetFPF< D0, D1, S0, Cast< double, FPNeg<  float, FPSub<  float, FPMul<  float, SF1, SF3, PPC64FPSCRRoundMode<S0> >, SF2, PPC64FPSCRRoundMode<S0> > > > >}}},


    // conversion

    {"frsp",    MASK_X10F, OPCODE_X10F(63,  12, 0), 1, {{OpClassCode::fCONV,  { R0, -1, -1}, {FPC, R1, -1, -1, -1, -1},       SetFP< D0,         Cast< double, CastFP< float, SD1, PPC64FPSCRRoundMode<S0> > > >}}},
    {"frsp.",   MASK_X10F, OPCODE_X10F(63,  12, 1), 1, {{OpClassCode::fCONV,  { R0,CR1, -1}, {FPC, R1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, Cast< double, CastFP< float, SD1, PPC64FPSCRRoundMode<S0> > > >}}},

    {"fctid",   MASK_X10F, OPCODE_X10F(63, 814, 0), 1, {{OpClassCode::fCONV,  { R0, -1, -1}, {FPC, R1, -1, -1, -1, -1},       SetFP< D0,         PPC64FPToInt< s64, SD1, PPC64FPSCRRoundMode<S0> > >}}},
    {"fctid.",  MASK_X10F, OPCODE_X10F(63, 814, 1), 1, {{OpClassCode::fCONV,  { R0,CR1, -1}, {FPC, R1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, PPC64FPToInt< s64, SD1, PPC64FPSCRRoundMode<S0> > >}}},

    {"fctidz",  MASK_X10F, OPCODE_X10F(63, 815, 0), 1, {{OpClassCode::fCONV,  { R0, -1, -1}, {FPC, R1, -1, -1, -1, -1},       SetFP< D0,         PPC64FPToInt< s64, SD1, IntConst<int, FE_TOWARDZERO> > >}}},
    {"fctidz.", MASK_X10F, OPCODE_X10F(63, 815, 1), 1, {{OpClassCode::fCONV,  { R0,CR1, -1}, {FPC, R1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, PPC64FPToInt< s64, SD1, IntConst<int, FE_TOWARDZERO> > >}}},

    {"fctiw",   MASK_X10F, OPCODE_X10F(63,  14, 0), 1, {{OpClassCode::fCONV,  { R0, -1, -1}, {FPC, R1, -1, -1, -1, -1},       SetFP< D0,         Cast< s64, PPC64FPToInt< s32, SD1, PPC64FPSCRRoundMode<S0> > > >}}},
    {"fctiw.",  MASK_X10F, OPCODE_X10F(63,  14, 1), 1, {{OpClassCode::fCONV,  { R0,CR1, -1}, {FPC, R1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, Cast< s64, PPC64FPToInt< s32, SD1, PPC64FPSCRRoundMode<S0> > > >}}},

    {"fctiwz",  MASK_X10F, OPCODE_X10F(63,  15, 0), 1, {{OpClassCode::fCONV,  { R0, -1, -1}, {FPC, R1, -1, -1, -1, -1},       SetFP< D0,         Cast< s64, PPC64FPToInt< s32, SD1, IntConst<int, FE_TOWARDZERO> > > >}}},
    {"fctiwz.", MASK_X10F, OPCODE_X10F(63,  15, 1), 1, {{OpClassCode::fCONV,  { R0,CR1, -1}, {FPC, R1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, Cast< s64, PPC64FPToInt< s32, SD1, IntConst<int, FE_TOWARDZERO> > > >}}},
    
    {"fcfid",   MASK_X10F, OPCODE_X10F(63, 846, 0), 1, {{OpClassCode::fCONV,  { R0, -1, -1}, {FPC, R1, -1, -1, -1, -1},       SetFP< D0,         Cast< double, Cast< s64, S1 > > >}}},
    {"fcfid.",  MASK_X10F, OPCODE_X10F(63, 846, 1), 1, {{OpClassCode::fCONV,  { R0,CR1, -1}, {FPC, R1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, Cast< double, Cast< s64, S1 > > >}}},

    // double型の中身を整数にする系
    {"frin",    MASK_X10F, OPCODE_X10F(63, 392, 0), 1, {{OpClassCode::fCONV,  { R0, -1, -1}, {FPC, R1, -1, -1, -1, -1},       SetFP< D0,         PPC64FRIN< SD1> >}}},
    {"frin.",   MASK_X10F, OPCODE_X10F(63, 392, 1), 1, {{OpClassCode::fCONV,  { R0,CR1, -1}, {FPC, R1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, PPC64FRIN< SD1> >}}},

    {"friz",    MASK_X10F, OPCODE_X10F(63, 424, 0), 1, {{OpClassCode::fCONV,  { R0, -1, -1}, {FPC, R1, -1, -1, -1, -1},       SetFP< D0,         PPC64FRIZ< SD1> >}}},
    {"friz.",   MASK_X10F, OPCODE_X10F(63, 424, 1), 1, {{OpClassCode::fCONV,  { R0,CR1, -1}, {FPC, R1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, PPC64FRIZ< SD1> >}}},

    {"frip",    MASK_X10F, OPCODE_X10F(63, 456, 0), 1, {{OpClassCode::fCONV,  { R0, -1, -1}, {FPC, R1, -1, -1, -1, -1},       SetFP< D0,         PPC64FRIP< SD1> >}}},
    {"frip.",   MASK_X10F, OPCODE_X10F(63, 456, 1), 1, {{OpClassCode::fCONV,  { R0,CR1, -1}, {FPC, R1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, PPC64FRIP< SD1> >}}},

    {"frim",    MASK_X10F, OPCODE_X10F(63, 488, 0), 1, {{OpClassCode::fCONV,  { R0, -1, -1}, {FPC, R1, -1, -1, -1, -1},       SetFP< D0,         PPC64FRIM< SD1> >}}},
    {"frim.",   MASK_X10F, OPCODE_X10F(63, 488, 1), 1, {{OpClassCode::fCONV,  { R0,CR1, -1}, {FPC, R1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, PPC64FRIM< SD1> >}}},

    // compare
    // <TODO> FPSCRのフラグセット
    {"fcmpu",   MASK_X10, OPCODE_X10(63,   0), 1, {{OpClassCode::fADD,  { R0, -1, -1}, {FPC, R1, R2, -1, -1, -1}, Set< D0, PPC64FPCompare< double, SD1, SD2 > >}}},
    {"fcmpo",   MASK_X10, OPCODE_X10(63,  32), 1, {{OpClassCode::fADD,  { R0, -1, -1}, {FPC, R1, R2, -1, -1, -1}, Set< D0, PPC64FPCompare< double, SD1, SD2 > >}}},

    // fsel

    // control register
    {"mffs",    MASK_X10F, OPCODE_X10F(63, 583, 0), 1, {{OpClassCode::iMOV,  { R0, -1, -1}, {FPC, -1, -1, -1, -1, -1},         Set< D0,         S0 >}}},
    {"mffs.",   MASK_X10F, OPCODE_X10F(63, 583, 1), 1, {{OpClassCode::iMOV,  { R0,CR1, -1}, {FPC, -1, -1, -1, -1, -1}, PPC64SetFPF< D0, D1, S0, S0 >}}},

    //{"mcrfs",   MASK_X10, OPCODE_X10F(63,  64), 1, {{OpClassCode::iMOV,  {FPC, R0, -1}, {FPC, I0, -1, -1, -1, -1}, PPC64MCRFS< D0, D1, S0, S1 >}}},

    {"mtfsfi",  MASK_X10F, OPCODE_X10F(63, 134, 0), 1, {{OpClassCode::iMOV,  {FPC, -1, -1}, {FPC, I0, I1, -1, -1, -1},  PPC64SetFPSCR< D0,     PPC64MTFSFI< S0, S1, S2 > >}}},
    {"mtfsfi.", MASK_X10F, OPCODE_X10F(63, 134, 1), 1, {{OpClassCode::iMOV,  {FPC,CR1, -1}, {FPC, I0, I1, -1, -1, -1}, PPC64SetFPSCRF< D0, D1, PPC64MTFSFI< S0, S1, S2 > >}}},
    
    {"mtfsf",   MASK_X10F, OPCODE_X10F(63, 711, 0), 1, {{OpClassCode::iALU,  {FPC, -1, -1}, {FPC, I0, R0, -1, -1, -1},  PPC64SetFPSCR< D0,     PPC64MTFSF< S0, S1, S2 > >}}},
    {"mtfsf.",  MASK_X10F, OPCODE_X10F(63, 711, 1), 1, {{OpClassCode::iALU,  {FPC, -1, -1}, {FPC, I0, R0, -1, -1, -1}, PPC64SetFPSCRF< D0, D1, PPC64MTFSF< S0, S1, S2 > >}}},

    {"mtfsb0",  MASK_X10F, OPCODE_X10F(63,  70, 0), 1, {{OpClassCode::iALU,  {FPC, -1, -1}, {FPC, I0, -1, -1, -1, -1},  PPC64SetFPSCR< D0,     BitAnd< u64, S0, RotateR< u32, IntConst< u32, (u32)0x7fffffff >, S1 > > >}}},
    {"mtfsb0.", MASK_X10F, OPCODE_X10F(63,  70, 1), 1, {{OpClassCode::iALU,  {FPC,CR1, -1}, {FPC, I0, -1, -1, -1, -1}, PPC64SetFPSCRF< D0, D1, BitAnd< u64, S0, RotateR< u32, IntConst< u32, (u32)0x7fffffff >, S1 > > >}}},
    {"mtfsb1",  MASK_X10F, OPCODE_X10F(63,  38, 0), 1, {{OpClassCode::iALU,  {FPC, -1, -1}, {FPC, I0, -1, -1, -1, -1},  PPC64SetFPSCR< D0,     BitOr < u64, S0, RotateR< u32, IntConst< u32, (u32)0x80000000 >, S1 > > >}}},
    {"mtfsb1.", MASK_X10F, OPCODE_X10F(63,  38, 1), 1, {{OpClassCode::iALU,  {FPC,CR1, -1}, {FPC, I0, -1, -1, -1, -1}, PPC64SetFPSCRF< D0, D1, BitOr < u64, S0, RotateR< u32, IntConst< u32, (u32)0x80000000 >, S1 > > >}}},


    //
    // branch
    //

    // gcc 4.2.1でコンパイルしたバイナリの場合
    // - 絶対分岐は使用されない？
    // - CALLにはbctrl のみ使用される？ (LK=1版はbctrlのみでよい？)
    // - blr, bctr の BH は 0のもののみ存在？
    {"b",       MASK_B, OPCODE_B(18, 0, 0), 1, {{OpClassCode::iBU,   { -1, -1, -1}, { I0, -1, -1, -1, -1, -1}, PPC64BranchRelUncond< S0 >}}},
    {"ba",      MASK_B, OPCODE_B(18, 1, 0), 1, {{OpClassCode::iBU,   { -1, -1, -1}, { I0, -1, -1, -1, -1, -1}, PPC64BranchAbsUncond< S0 >}}},
    {"bl",      MASK_B, OPCODE_B(18, 0, 1), 1, {{OpClassCode::CALL,  { LR, -1, -1}, { I0, -1, -1, -1, -1, -1}, PPC64CallRelUncond< D0, S0 >}}},
    {"bla",     MASK_B, OPCODE_B(18, 1, 1), 1, {{OpClassCode::CALL,  { LR, -1, -1}, { I0, -1, -1, -1, -1, -1}, PPC64CallAbsUncond< D0, S0 >}}},

    {"bf",      MASK_BC(034), OPCODE_BC(16, 0, 0, 004), 1, {{OpClassCode::iBC,  { -1, -1, -1}, { I1, R0, I0, -1, -1, -1}, PPC64BranchRelCond< S0, Compare< PPC64CRBit<S1, S2>, IntConst<u64, 0>, IntCondEqual<u64> > >}}},
    {"bt",      MASK_BC(034), OPCODE_BC(16, 0, 0, 014), 1, {{OpClassCode::iBC,  { -1, -1, -1}, { I1, R0, I0, -1, -1, -1}, PPC64BranchRelCond< S0, Compare< PPC64CRBit<S1, S2>, IntConst<u64, 0>, IntCondNotEqual<u64> > >}}},
    {"bdnzf",   MASK_BC(036), OPCODE_BC(16, 0, 0, 000), 1, {{OpClassCode::iBC,  {CTR, -1, -1}, { I1, R0, I0,CTR, -1, -1}, 
        PPC64BranchRelCond< S0, CondAnd< 
            Compare< PPC64DecCTR< D0, S3 >, IntConst<u64, 0>, IntCondNotEqual<u64> >,
            Compare< PPC64CRBit<S1, S2>, IntConst<u64, 0>, IntCondEqual<u64> >
        > >
    }}},
    {"bdzf",    MASK_BC(036), OPCODE_BC(16, 0, 0, 002), 1, {{OpClassCode::iBC,  {CTR, -1, -1}, { I1, R0, I0,CTR, -1, -1}, 
        PPC64BranchRelCond< S0, CondAnd< 
            Compare< PPC64DecCTR< D0, S3 >, IntConst<u64, 0>, IntCondEqual<u64> >,
            Compare< PPC64CRBit<S1, S2>, IntConst<u64, 0>, IntCondEqual<u64> >
        > >
    }}},
    {"bdnzt",   MASK_BC(036), OPCODE_BC(16, 0, 0, 010), 1, {{OpClassCode::iBC,  {CTR, -1, -1}, { I1, R0, I0,CTR, -1, -1}, 
        PPC64BranchRelCond< S0, CondAnd< 
            Compare< PPC64DecCTR< D0, S3 >, IntConst<u64, 0>, IntCondNotEqual<u64> >,
            Compare< PPC64CRBit<S1, S2>, IntConst<u64, 0>, IntCondNotEqual<u64> >
        > >
    }}},
    {"bdzt",    MASK_BC(036), OPCODE_BC(16, 0, 0, 012), 1, {{OpClassCode::iBC,  {CTR, -1, -1}, { I1, R0, I0,CTR, -1, -1}, 
        PPC64BranchRelCond< S0, CondAnd< 
            Compare< PPC64DecCTR< D0, S3 >, IntConst<u64, 0>, IntCondEqual<u64> >,
            Compare< PPC64CRBit<S1, S2>, IntConst<u64, 0>, IntCondNotEqual<u64> >
        > >
    }}},
    {"bdnz",    MASK_BC(026), OPCODE_BC(16, 0, 0, 020), 1, {{OpClassCode::iBC,  {CTR, -1, -1}, { I1,CTR, -1, -1, -1, -1}, PPC64BranchRelCond< S0, Compare< PPC64DecCTR< D0, S1 >, IntConst<u64, 0>, IntCondNotEqual<u64> > >}}},
    {"bdz",     MASK_BC(026), OPCODE_BC(16, 0, 0, 022), 1, {{OpClassCode::iBC,  {CTR, -1, -1}, { I1,CTR, -1, -1, -1, -1}, PPC64BranchRelCond< S0, Compare< PPC64DecCTR< D0, S1 >, IntConst<u64, 0>, IntCondEqual<u64> > >}}},

    {"bctr",    MASK_BCLR(024), OPCODE_BCLR(19,528, 0, 024, 0), 1, {{OpClassCode::iJUMP, { -1, -1, -1}, {CTR, -1, -1, -1, -1, -1}, PPC64BranchAbsUncond<  S0 >}}},
    {"bctrl",   MASK_BCLR(024), OPCODE_BCLR(19,528, 1, 024, 0), 1, {{OpClassCode::CALL_JUMP,  { LR, -1, -1}, {CTR, -1, -1, -1, -1, -1}, PPC64CallAbsUncond< D0, S0 >}}},

    {"blr",     MASK_BCLR(024), OPCODE_BCLR(19, 16, 0, 024, 0), 1, {{OpClassCode::RET,   { -1, -1, -1}, { LR, -1, -1, -1, -1, -1}, PPC64BranchAbsUncond< S0 >}}},

    {"bflr",    MASK_BCLR(034), OPCODE_BCLR(19, 16, 0, 004, 0), 1, {{OpClassCode::RETC,  { -1, -1, -1}, { LR, R0, I0, -1, -1, -1}, PPC64BranchAbsCond< S0, Compare< PPC64CRBit<S1, S2>, IntConst<u64, 0>, IntCondEqual<u64> > >}}},
    {"btlr",    MASK_BCLR(034), OPCODE_BCLR(19, 16, 0, 014, 0), 1, {{OpClassCode::RETC,  { -1, -1, -1}, { LR, R0, I0, -1, -1, -1}, PPC64BranchAbsCond< S0, Compare< PPC64CRBit<S1, S2>, IntConst<u64, 0>, IntCondNotEqual<u64> > >}}},
    {"bdnzflr", MASK_BCLR(036), OPCODE_BCLR(19, 16, 0, 000, 0), 1, {{OpClassCode::RETC,  {CTR, -1, -1}, { LR, R0, I0,CTR, -1, -1}, 
        PPC64BranchAbsCond< S0, CondAnd< 
            Compare< PPC64DecCTR< D0, S3 >, IntConst<u64, 0>, IntCondNotEqual<u64> >,
            Compare< PPC64CRBit<S1, S2>, IntConst<u64, 0>, IntCondEqual<u64> >
        > >
    }}},
    {"bdzflr",  MASK_BCLR(036), OPCODE_BCLR(19, 16, 0, 002, 0), 1, {{OpClassCode::RETC,  {CTR, -1, -1}, { LR, R0, I0,CTR, -1, -1}, 
        PPC64BranchAbsCond< S0, CondAnd< 
            Compare< PPC64DecCTR< D0, S3 >, IntConst<u64, 0>, IntCondEqual<u64> >,
            Compare< PPC64CRBit<S1, S2>, IntConst<u64, 0>, IntCondEqual<u64> >
        > >
    }}},
    {"bdnztlr", MASK_BCLR(036), OPCODE_BCLR(19, 16, 0, 010, 0), 1, {{OpClassCode::RETC,  {CTR, -1, -1}, { LR, R0, I0,CTR, -1, -1}, 
        PPC64BranchAbsCond< S0, CondAnd< 
            Compare< PPC64DecCTR< D0, S3 >, IntConst<u64, 0>, IntCondNotEqual<u64> >,
            Compare< PPC64CRBit<S1, S2>, IntConst<u64, 0>, IntCondNotEqual<u64> >
        > >
    }}},
    {"bdztlr",  MASK_BCLR(036), OPCODE_BCLR(19, 16, 0, 012, 0), 1, {{OpClassCode::RETC,  {CTR, -1, -1}, { LR, R0, I0,CTR, -1, -1}, 
        PPC64BranchAbsCond< S0, CondAnd< 
            Compare< PPC64DecCTR< D0, S3 >, IntConst<u64, 0>, IntCondEqual<u64> >,
            Compare< PPC64CRBit<S1, S2>, IntConst<u64, 0>, IntCondNotEqual<u64> >
        > >
    }}},
    {"bdnzlr",  MASK_BCLR(026), OPCODE_BCLR(19, 16, 0, 020, 0), 1, {{OpClassCode::RETC,  {CTR, -1, -1}, { LR,CTR, -1, -1, -1, -1}, PPC64BranchAbsCond< S0, Compare< PPC64DecCTR< D0, S1 >, IntConst<u64, 0>, IntCondNotEqual<u64> > >}}},
    {"bdzlr",   MASK_BCLR(026), OPCODE_BCLR(19, 16, 0, 022, 0), 1, {{OpClassCode::RETC,  {CTR, -1, -1}, { LR,CTR, -1, -1, -1, -1}, PPC64BranchAbsCond< S0, Compare< PPC64DecCTR< D0, S1 >, IntConst<u64, 0>, IntCondEqual<u64> > >}}},

    //
    // conditional register logical instructions
    //
    {"crand",   MASK_X10, OPCODE_X10(19, 257), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R0, I0, R1, I1, R2, I2}, PPC64SetCRBit< D0, S1, S0, PPC64CRAnd< S2, S3, S4, S5> >}}},
    {"crnand",  MASK_X10, OPCODE_X10(19, 225), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R0, I0, R1, I1, R2, I2}, PPC64SetCRBit< D0, S1, S0, PPC64CRNand< S2, S3, S4, S5> >}}},
    {"cror",    MASK_X10, OPCODE_X10(19, 449), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R0, I0, R1, I1, R2, I2}, PPC64SetCRBit< D0, S1, S0, PPC64CROr< S2, S3, S4, S5> >}}},
    {"crxor",   MASK_X10, OPCODE_X10(19, 193), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R0, I0, R1, I1, R2, I2}, PPC64SetCRBit< D0, S1, S0, PPC64CRXor< S2, S3, S4, S5> >}}},
    {"crnor",   MASK_X10, OPCODE_X10(19,  33), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R0, I0, R1, I1, R2, I2}, PPC64SetCRBit< D0, S1, S0, PPC64CRNor< S2, S3, S4, S5> >}}},
    {"creqv",   MASK_X10, OPCODE_X10(19, 289), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R0, I0, R1, I1, R2, I2}, PPC64SetCRBit< D0, S1, S0, PPC64CREqv< S2, S3, S4, S5> >}}},
    {"crandc",  MASK_X10, OPCODE_X10(19, 129), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R0, I0, R1, I1, R2, I2}, PPC64SetCRBit< D0, S1, S0, PPC64CRAndC< S2, S3, S4, S5> >}}},
    {"crorc",   MASK_X10, OPCODE_X10(19, 417), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R0, I0, R1, I1, R2, I2}, PPC64SetCRBit< D0, S1, S0, PPC64CROrC< S2, S3, S4, S5> >}}},

    {"mcrf",    MASK_X10, OPCODE_X10(19,   0), 1, {{OpClassCode::iALU,  { R0, -1, -1}, { R1, -1, -1, -1, -1, -1}, Set< D0, S0 >}}},

    // instruction synchronize : シングルスレッドなので無視
    {"isync",   MASK_X10, OPCODE_X10(19, 150), 1, {{OpClassCode::iNOP,  { -1, -1, -1}, { -1, -1, -1, -1, -1, -1}, NoOperation}}},
    
    //// 実際に同期する場合
    //{"isync",   MASK_X10, OPCODE_X10(19, 150), 1, {{OpClassCode::syscall,  { -1, -1, -1}, { -1, -1, -1, -1, -1, -1}, NoOperation}}},

    // memory barrier : シングルスレッドなので無視
    {"sync",    MASK_X10, OPCODE_X10(31, 598), 1, {{OpClassCode::iNOP,  { -1, -1, -1}, { -1, -1, -1, -1, -1, -1}, NoOperation}}},
    {"eieio",   MASK_X10, OPCODE_X10(31, 854), 1, {{OpClassCode::iNOP,  { -1, -1, -1}, { -1, -1, -1, -1, -1, -1}, NoOperation}}},

    //
    // cache management
    //

    // cache management 命令で使用するキャッシュブロックサイズは auxv, AT_DCACHEBSIZE から取得
    // cf.) glibc/sysdeps/unix/sysv/linux/powerpc/libc-start.c
    // これを設定しない（0にする）と，glibcでdcbzを使用する唯一の関数であるmemsetがdcbzを使用しなくなる

    // 無視する
    // data cache block touch
    {"dcbt",    MASK_X10, OPCODE_X10(31, 278), 1, {{OpClassCode::iNOP,  { -1, -1, -1}, { -1, -1, -1, -1, -1, -1}, NoOperation}}},
    // data cache block touch for store
    {"dcbtst",  MASK_X10, OPCODE_X10(31, 246), 1, {{OpClassCode::iNOP,  { -1, -1, -1}, { -1, -1, -1, -1, -1, -1}, NoOperation}}},

    // data cache block set to zero : 実際に0をストア
    //{"dcbz",    MASK_X10, OPCODE_X10(31,1014), 1, {{OpClassCode::iST,  { -1, -1, -1}, { -1, -1, -1, -1, -1, -1}, }}},
};

//
// split load Store
//
PPC64Converter::OpDef PPC64Converter::m_OpDefsSplitLoadStore[] =
{
    {}
};

//
// non-split load Store
//
PPC64Converter::OpDef PPC64Converter::m_OpDefsNonSplitLoadStore[] =
{
    // <FIXME> RA=0版? 多分使われないけど
    // integer load
    {"lbz",   MASK_OP, OPCODE_OP(34), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1}, Set< D0,                Load< u8,     IntAdd< u64, S0, S1 > > >}
    }},
    {"lbzu",  MASK_OP, OPCODE_OP(35), 1, {
        {OpClassCode::iLD,  { R0, R1, -1}, { R1, I0, -1, -1, -1, -1}, Set< D0, PPC64LoadWithUpdate< u8, D1, IntAdd< u64, S0, S1 > > >}
    }},
    {"lbzx",  MASK_X10, OPCODE_X10(31,  87), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1}, Set< D0,                Load< u8,     IntAdd< u64, S0, S1 > > >}
    }},
    {"lbzx",  MASK_X10|MASK_RA, OPCODE_X10(31,  87), 1, {   // RA=0
        {OpClassCode::iLD,  { R0, -1, -1}, { R2, -1, -1, -1, -1, -1}, Set< D0,                Load< u8,     S0 > >}
    }},
    {"lbzux", MASK_X10, OPCODE_X10(31, 119), 1, {
        {OpClassCode::iLD,  { R0, R1, -1}, { R1, R2, -1, -1, -1, -1}, Set< D0, PPC64LoadWithUpdate< u8, D1, IntAdd< u64, S0, S1 > > >}
    }},

    {"lhz",   MASK_OP, OPCODE_OP(40), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1}, Set< D0,                Load< u16,     IntAdd< u64, S0, S1 > > >}
    }},
    {"lhzu",  MASK_OP, OPCODE_OP(41), 1, {
        {OpClassCode::iLD,  { R0, R1, -1}, { R1, I0, -1, -1, -1, -1}, Set< D0, PPC64LoadWithUpdate< u16, D1, IntAdd< u64, S0, S1 > > >}
    }},
    {"lhzx",  MASK_X10, OPCODE_X10(31, 279), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1}, Set< D0,                Load< u16,     IntAdd< u64, S0, S1 > > >}
    }},
    {"lhzx",  MASK_X10|MASK_RA, OPCODE_X10(31, 279), 1, {   // RA=0
        {OpClassCode::iLD,  { R0, -1, -1}, { R2, -1, -1, -1, -1, -1}, Set< D0,                Load< u16,     S0 > >}
    }},
    {"lhzux", MASK_X10, OPCODE_X10(31, 311), 1, {
        {OpClassCode::iLD,  { R0, R1, -1}, { R1, R2, -1, -1, -1, -1}, Set< D0, PPC64LoadWithUpdate< u16, D1, IntAdd< u64, S0, S1 > > >}
    }},

    {"lha",   MASK_OP, OPCODE_OP(42), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1}, SetSext< D0,                Load< u16,     IntAdd< u64, S0, S1 > > >}
    }},
    {"lhau",  MASK_OP, OPCODE_OP(43), 1, {
        {OpClassCode::iLD,  { R0, R1, -1}, { R1, I0, -1, -1, -1, -1}, SetSext< D0, PPC64LoadWithUpdate< u16, D1, IntAdd< u64, S0, S1 > > >}
    }},
    {"lhax",  MASK_X10, OPCODE_X10(31, 343), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1}, SetSext< D0,                Load< u16,     IntAdd< u64, S0, S1 > > >}
    }},
    {"lhax",  MASK_X10|MASK_RA, OPCODE_X10(31, 343), 1, {   // RA=0
        {OpClassCode::iLD,  { R0, -1, -1}, { R2, -1, -1, -1, -1, -1}, SetSext< D0,                Load< u16,     S0 > >}
    }},
    {"lhaux", MASK_X10, OPCODE_X10(31, 375), 1, {
        {OpClassCode::iLD,  { R0, R1, -1}, { R1, R2, -1, -1, -1, -1}, SetSext< D0, PPC64LoadWithUpdate< u16, D1, IntAdd< u64, S0, S1 > > >}
    }},

    {"lwz",   MASK_OP, OPCODE_OP(32), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1}, Set< D0,                Load< u32,     IntAdd< u64, S0, S1 > > >}
    }},
    {"lwzu",  MASK_OP, OPCODE_OP(33), 1, {
        {OpClassCode::iLD,  { R0, R1, -1}, { R1, I0, -1, -1, -1, -1}, Set< D0, PPC64LoadWithUpdate< u32, D1, IntAdd< u64, S0, S1 > > >}
    }},
    {"lwzx",  MASK_X10, OPCODE_X10(31,  23), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1}, Set< D0,                Load< u32,     IntAdd< u64, S0, S1 > > >}
    }},
    {"lwzx",  MASK_X10|MASK_RA, OPCODE_X10(31,  23), 1, {   // RA=0
        {OpClassCode::iLD,  { R0, -1, -1}, { R2, -1, -1, -1, -1, -1}, Set< D0,                Load< u32,     S0 > >}
    }},
    {"lwzux", MASK_X10, OPCODE_X10(31,  55), 1, {
        {OpClassCode::iLD,  { R0, R1, -1}, { R1, R2, -1, -1, -1, -1}, Set< D0, PPC64LoadWithUpdate< u32, D1, IntAdd< u64, S0, S1 > > >}
    }},

    {"lwa",   MASK_X2, OPCODE_X2(58, 2), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1}, SetSext< D0,                Load< u32,     IntAdd< u64, S0, LShiftL< s64, S1, IntConst<unsigned int, 2>, 63 > > > >}
    }},
    {"lwax",  MASK_X10, OPCODE_X10(31, 341), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1}, SetSext< D0,                Load< u32,     IntAdd< u64, S0, S1 > > >}
    }},
    {"lwax",  MASK_X10|MASK_RA, OPCODE_X10(31, 341), 1, {   // RA=0
        {OpClassCode::iLD,  { R0, -1, -1}, { R2, -1, -1, -1, -1, -1}, SetSext< D0,                Load< u32,     S0 > >}
    }},
    {"lwaux", MASK_X10, OPCODE_X10(31, 373), 1, {
        {OpClassCode::iLD,  { R0, R1, -1}, { R1, R2, -1, -1, -1, -1}, SetSext< D0, PPC64LoadWithUpdate< u32, D1, IntAdd< u64, S0, S1 > > >}
    }},

    {"ld",    MASK_X2, OPCODE_X2(58, 0), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1}, Set< D0,                Load< u64,     IntAdd< u64, S0, LShiftL< s64, S1, IntConst<unsigned int, 2>, 63 > > > >}
    }},
    {"ldu",   MASK_X2, OPCODE_X2(58, 1), 1, {
        {OpClassCode::iLD,  { R0, R1, -1}, { R1, I0, -1, -1, -1, -1}, Set< D0, PPC64LoadWithUpdate< u64, D1, IntAdd< u64, S0, LShiftL< s64, S1, IntConst<unsigned int, 2>, 63 > > > >}
    }},
    {"ldx",   MASK_X10, OPCODE_X10(31,  21), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1}, Set< D0,                Load< u64,     IntAdd< u64, S0, S1 > > >}
    }},
    {"ldx",   MASK_X10|MASK_RA, OPCODE_X10(31,  21), 1, {   // RA=0
        {OpClassCode::iLD,  { R0, -1, -1}, { R2, -1, -1, -1, -1, -1}, Set< D0,                Load< u64,     S0 > >}
    }},
    {"ldux",  MASK_X10, OPCODE_X10(31,  53), 1, {
        {OpClassCode::iLD,  { R0, R1, -1}, { R1, R2, -1, -1, -1, -1}, Set< D0, PPC64LoadWithUpdate< u64, D1, IntAdd< u64, S0, S1 > > >}
    }},

    // integer store
    {"stb",   MASK_OP, OPCODE_OP(38), 1, {
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R1, I0, -1, -1, -1},                Store< u8,     S0, IntAdd< u64, S1, S2 > >}
    }},
    {"stbu",  MASK_OP, OPCODE_OP(39), 1, {
        {OpClassCode::iST,  { R1, -1, -1}, { R0, R1, I0, -1, -1, -1}, PPC64StoreWithUpdate< u8, D0, S0, IntAdd< u64, S1, S2 > >}
    }},
    {"stbx",  MASK_X10, OPCODE_X10(31, 215), 1, {
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R1, R2, -1, -1, -1},                Store< u8,     S0, IntAdd< u64, S1, S2 > >}
    }},
    {"stbx",  MASK_X10|MASK_RA, OPCODE_X10(31, 215), 1, {   // RA=0
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R2, -1, -1, -1, -1},                Store< u8,     S0, S1 >}
    }},
    {"stbux", MASK_X10, OPCODE_X10(31, 247), 1, {
        {OpClassCode::iST,  { R1, -1, -1}, { R0, R1, R2, -1, -1, -1}, PPC64StoreWithUpdate< u8, D0, S0, IntAdd< u64, S1, S2 > >}
    }},

    {"sth",   MASK_OP, OPCODE_OP(44), 1, {
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R1, I0, -1, -1, -1},                Store< u16,     S0, IntAdd< u64, S1, S2 > >}
    }},
    {"sthu",  MASK_OP, OPCODE_OP(45), 1, {
        {OpClassCode::iST,  { R1, -1, -1}, { R0, R1, I0, -1, -1, -1}, PPC64StoreWithUpdate< u16, D0, S0, IntAdd< u64, S1, S2 > >}
    }},
    {"sthx",  MASK_X10, OPCODE_X10(31, 407), 1, {
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R1, R2, -1, -1, -1},                Store< u16,     S0, IntAdd< u64, S1, S2 > >}
    }},
    {"sthx",  MASK_X10|MASK_RA, OPCODE_X10(31, 407), 1, {   // RA=0
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R2, -1, -1, -1, -1},                Store< u16,     S0, S1 >}
    }},
    {"sthux", MASK_X10, OPCODE_X10(31, 439), 1, {
        {OpClassCode::iST,  { R1, -1, -1}, { R0, R1, R2, -1, -1, -1}, PPC64StoreWithUpdate< u16, D0, S0, IntAdd< u64, S1, S2 > >}
    }},

    {"stw",   MASK_OP, OPCODE_OP(36), 1, {
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R1, I0, -1, -1, -1},                Store< u32,     S0, IntAdd< u64, S1, S2 > >}
    }},
    {"stwu",  MASK_OP, OPCODE_OP(37), 1, {
        {OpClassCode::iST,  { R1, -1, -1}, { R0, R1, I0, -1, -1, -1}, PPC64StoreWithUpdate< u32, D0, S0, IntAdd< u64, S1, S2 > >}
    }},
    {"stwx",  MASK_X10, OPCODE_X10(31, 151), 1, {
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R1, R2, -1, -1, -1},                Store< u32,     S0, IntAdd< u64, S1, S2 > >}
    }},
    {"stwx",  MASK_X10|MASK_RA, OPCODE_X10(31, 151), 1, {   // RA=0
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R2, -1, -1, -1, -1},                Store< u32,     S0, S1 >}
    }},
    {"stwux", MASK_X10, OPCODE_X10(31, 183), 1, {
        {OpClassCode::iST,  { R1, -1, -1}, { R0, R1, R2, -1, -1, -1}, PPC64StoreWithUpdate< u32, D0, S0, IntAdd< u64, S1, S2 > >}
    }},

    {"std",   MASK_X2, OPCODE_X2(62, 0), 1, {
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R1, I0, -1, -1, -1},                Store< u64,     S0, IntAdd< u64, S1, LShiftL< s64, S2, IntConst<unsigned int, 2>, 63 > > >}
    }},
    {"stdu",  MASK_X2, OPCODE_X2(62, 1), 1, {
        {OpClassCode::iST,  { R1, -1, -1}, { R0, R1, I0, -1, -1, -1}, PPC64StoreWithUpdate< u64, D0, S0, IntAdd< u64, S1, LShiftL< s64, S2, IntConst<unsigned int, 2>, 63 > > >}
    }},
    {"stdx",  MASK_X10, OPCODE_X10(31, 149), 1, {
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R1, R2, -1, -1, -1},                Store< u64,     S0, IntAdd< u64, S1, S2 > >}
    }},
    {"stdx",  MASK_X10|MASK_RA, OPCODE_X10(31, 149), 1, {   // RA=0
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R2, -1, -1, -1, -1},                Store< u64,     S0, S1 >}
    }},
    {"stdux", MASK_X10, OPCODE_X10(31, 181), 1, {
        {OpClassCode::iST,  { R1, -1, -1}, { R0, R1, R2, -1, -1, -1}, PPC64StoreWithUpdate< u64, D0, S0, IntAdd< u64, S1, S2 > >}
    }},

    // FP load
    {"lfs",   MASK_OP, OPCODE_OP(48), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1}, SetFP< D0, Cast<double, AsFP< float,                Load< u32,     IntAdd< u64, S0, S1 > > > > >}
    }},
    {"lfsu",  MASK_OP, OPCODE_OP(49), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1}, SetFP< D0, Cast<double, AsFP< float, PPC64LoadWithUpdate< u32, D1, IntAdd< u64, S0, S1 > > > > >}
    }},
    {"lfsx",  MASK_X10, OPCODE_X10(31, 535), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1}, SetFP< D0, Cast<double, AsFP< float,                Load< u32,     IntAdd< u64, S0, S1 > > > > >}
    }},
    {"lfsx",  MASK_X10|MASK_RA, OPCODE_X10(31, 535), 1, {   // RA=0
        {OpClassCode::iLD,  { R0, -1, -1}, { R2, -1, -1, -1, -1, -1}, SetFP< D0, Cast<double, AsFP< float,                Load< u32,     S0 > > > >}
    }},
    {"lfsux", MASK_X10, OPCODE_X10(31, 567), 1, {
        {OpClassCode::iLD,  { R0, R1, -1}, { R1, R2, -1, -1, -1, -1}, SetFP< D0, Cast<double, AsFP< float, PPC64LoadWithUpdate< u32, D1, IntAdd< u64, S0, S1 > > > > >}
    }},

    {"lfd",   MASK_OP, OPCODE_OP(50), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1}, Set< D0,                Load< u64,     IntAdd< u64, S0, S1 > > >}
    }},
    {"lfdu",  MASK_OP, OPCODE_OP(51), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, I0, -1, -1, -1, -1}, Set< D0, PPC64LoadWithUpdate< u64, D1, IntAdd< u64, S0, S1 > > >}
    }},
    {"lfdx",  MASK_X10, OPCODE_X10(31, 599), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1}, Set< D0,                Load< u64,     IntAdd< u64, S0, S1 > > >}
    }},
    {"lfdx",  MASK_X10|MASK_RA, OPCODE_X10(31, 599), 1, {   // RA=0
        {OpClassCode::iLD,  { R0, -1, -1}, { R2, -1, -1, -1, -1, -1}, Set< D0,                Load< u64,     S0 > >}
    }},
    {"lfdux", MASK_X10, OPCODE_X10(31, 631), 1, {
        {OpClassCode::iLD,  { R0, R1, -1}, { R1, R2, -1, -1, -1, -1}, Set< D0, PPC64LoadWithUpdate< u64, D1, IntAdd< u64, S0, S1 > > >}
    }},

    // FP store
    {"stfs",  MASK_OP, OPCODE_OP(52), 1, {
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R1, I0, -1, -1, -1},                Store< u32,     AsInt< u32, SF0>, IntAdd< u64, S1, S2 > >}
    }},
    {"stfsu", MASK_OP, OPCODE_OP(53), 1, {
        {OpClassCode::iST,  { R1, -1, -1}, { R0, R1, I0, -1, -1, -1}, PPC64StoreWithUpdate< u32, D0, AsInt< u32, SF0>, IntAdd< u64, S1, S2 > >}
    }},
    {"stfsx", MASK_X10, OPCODE_X10(31, 663), 1, {
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R1, R2, -1, -1, -1},                Store< u32,     AsInt< u32, SF0>, IntAdd< u64, S1, S2 > >}
    }},
    {"stfsx", MASK_X10|MASK_RA, OPCODE_X10(31, 663), 1, {   // RA=0
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R1, R2, -1, -1, -1},                Store< u32,     AsInt< u32, SF0>, S1 >}
    }},
    {"stfsux",MASK_X10, OPCODE_X10(31, 695), 1, {
        {OpClassCode::iST,  { R1, -1, -1}, { R0, R1, R2, -1, -1, -1}, PPC64StoreWithUpdate< u32, D0, AsInt< u32, SF0>, IntAdd< u64, S1, S2 > >}
    }},

    {"stfd",  MASK_OP, OPCODE_OP(54), 1, {
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R1, I0, -1, -1, -1},                Store< u64,     S0, IntAdd< u64, S1, S2 > >}
    }},
    {"stfdu", MASK_OP, OPCODE_OP(55), 1, {
        {OpClassCode::iST,  { R1, -1, -1}, { R0, R1, I0, -1, -1, -1}, PPC64StoreWithUpdate< u64, D0, S0, IntAdd< u64, S1, S2 > >}
    }},
    {"stfdx", MASK_X10, OPCODE_X10(31, 727), 1, {
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R1, R2, -1, -1, -1},                Store< u64,     S0, IntAdd< u64, S1, S2 > >}
    }},
    {"stfdx", MASK_X10|MASK_RA, OPCODE_X10(31, 727), 1, {   // RA=0
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R2, -1, -1, -1, -1},                Store< u64,     S0, S1 >}
    }},
    {"stfdux",MASK_X10, OPCODE_X10(31, 759), 1, {
        {OpClassCode::iST,  { R1, -1, -1}, { R0, R1, R2, -1, -1, -1}, PPC64StoreWithUpdate< u64, D0, S0, IntAdd< u64, S1, S2 > >}
    }},

    {"stfiwx", MASK_X10, OPCODE_X10(31, 983), 1, {
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R1, R2, -1, -1, -1},                Store< u32,     S0, IntAdd< u64, S1, S2 > >}
    }},
    {"stfiwx", MASK_X10|MASK_RA, OPCODE_X10(31, 983), 1, {  // RA=0
        {OpClassCode::iST,  { -1, -1, -1}, { R0, R2, -1, -1, -1, -1},                Store< u32,     S0, S1 >}
    }},

    // lwarx, stwcxは必ず成功する
    {"lwarx",  MASK_X10, OPCODE_X10(31,  20), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1}, Set< D0,                Load< u32,     IntAdd< u64, S0, S1 > > >}
    }},
    {"lwarx",  MASK_X10|MASK_RA, OPCODE_X10(31,  20), 1, {  // RA=0
        {OpClassCode::iLD,  { R0, -1, -1}, { R2, -1, -1, -1, -1, -1}, Set< D0,                Load< u32,     S0 > >}
    }},
    {"stwcx",  MASK_X10, OPCODE_X10(31, 150), 1, {
        {OpClassCode::iST,  {CR0, -1, -1}, { R0, R1, R2, -1, -1, -1},
            Sequence2<
                Store< u32, S0, IntAdd< u64, S1, S2 > >,
                Set< D0, IntConst<u64, 0x2> >
            >
        }
    }},
    {"stwcx.",  MASK_X10|MASK_RA, OPCODE_X10(31, 150), 1, { // RA=0
        {OpClassCode::iST,  {CR0, -1, -1}, { R0, R2, -1, -1, -1, -1},
            Sequence2<
                Store< u32, S0, S1 >,
                Set< D0, IntConst<u64, 0x2> >
            >
        }
    }},

    {"ldarx",  MASK_X10, OPCODE_X10(31,  84), 1, {
        {OpClassCode::iLD,  { R0, -1, -1}, { R1, R2, -1, -1, -1, -1}, Set< D0,                Load< u64,     IntAdd< u64, S0, S1 > > >}
    }},
    {"ldarx",  MASK_X10|MASK_RA, OPCODE_X10(31,  84), 1, {  // RA=0
        {OpClassCode::iLD,  { R0, -1, -1}, { R2, -1, -1, -1, -1, -1}, Set< D0,                Load< u64,     S0 > >}
    }},
    {"stdcx.",  MASK_X10, OPCODE_X10(31, 214), 1, {
        {OpClassCode::iST,  {CR0, -1, -1}, { R0, R1, R2, -1, -1, -1},
            Sequence2<
                Store< u64, S0, IntAdd< u64, S1, S2 > >,
                Set< D0, IntConst<u64, 0x2> >
            >
        }
    }},
    {"stdcx",  MASK_X10|MASK_RA, OPCODE_X10(31, 214), 1, {  // RA=0
        {OpClassCode::iST,  {CR0, -1, -1}, { R0, R2, -1, -1, -1, -1},
            Sequence2<
                Store< u64, S0, S1 >,
                Set< D0, IntConst<u64, 0x2> >
            >
        }
    }},};


//
// PPC64Converter
//


PPC64Converter::PPC64Converter()
{
    AddToOpMap(m_OpDefsBase, sizeof(m_OpDefsBase)/sizeof(OpDef));
    if (IsSplitLoadStoreEnabled()) {
        // <TODO> split load/store
        cerr << "warning: split load/store not yet implemented" << endl;
        AddToOpMap(m_OpDefsNonSplitLoadStore, sizeof(m_OpDefsNonSplitLoadStore)/sizeof(OpDef));
        //AddToOpMap(m_OpDefsSplitLoadStore, sizeof(m_OpDefsSplitLoadStore)/sizeof(OpDef));
    }
    else{
        AddToOpMap(m_OpDefsNonSplitLoadStore, sizeof(m_OpDefsNonSplitLoadStore)/sizeof(OpDef));
    }
}

PPC64Converter::~PPC64Converter()
{
}

// srcTemplate に対応するオペランドの種類と，レジスタならば番号を，即値ならばindexを返す
std::pair<PPC64Converter::OperandType, int> PPC64Converter::GetActualSrcOperand(int srcTemplate, const DecodedInsn& decoded) const
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
int PPC64Converter::GetActualRegNumber(int regTemplate, const DecodedInsn& decoded) const
{
    if (regTemplate == -1) {
        return -1;
    }
    else if (RegTemplateBegin <= regTemplate && regTemplate <= RegTemplateEnd) {
        return decoded.Reg[regTemplate - RegTemplateBegin];
    }
    else if (0 <= regTemplate && regTemplate < PPC64Info::RegisterCount) {
        return regTemplate;
    }
    else {
        ASSERT(0, "PPC64Converter Logic Error : invalid regTemplate");
        return -1;
    }
}

// レジスタ番号regNumがゼロレジスタかどうかを判定する
bool PPC64Converter::IsZeroReg(int regNum) const
{
    // PowerPC にはゼロレジスタがない
    return false;
}


void PPC64Converter::PPC64UnknownOperation(OpEmulationState* opState)
{
    u32 codeWord = static_cast<u32>( SrcOperand<0>()(opState) );

    DecoderType decoder;
    DecodedInsn decoded;
    decoder.Decode( codeWord, &decoded);

    stringstream ss;
    u32 opcode = (codeWord >> 26) & 0x3f;
    ss << "unknown instruction : " << hex << setw(8) << codeWord << endl;
    ss << "\topcode : " << hex << opcode << endl;
    ss << "\tregs : " << hex;
    copy(decoded.Reg.begin(), decoded.Reg.end(), ostream_iterator<int>(ss, ", "));
    ss << endl;
    ss << "\timms : " << hex;
    copy(decoded.Imm.begin(), decoded.Imm.end(), ostream_iterator<u64>(ss, ", "));
    ss << endl;

    THROW_RUNTIME_ERROR(ss.str().c_str());
}

const PPC64Converter::OpDef* PPC64Converter::GetOpDefUnknown() const
{
    return &m_OpDefUnknown;
}
