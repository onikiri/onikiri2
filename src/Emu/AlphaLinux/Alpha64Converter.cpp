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

#include "Emu/AlphaLinux/Alpha64Converter.h"

#include "Emu/Utility/GenericOperation.h"
#include "Emu/Utility/OpEmulationState.h"

#include "Emu/AlphaLinux/Alpha64Info.h"
#include "Emu/AlphaLinux/Alpha64OpInfo.h"
#include "Emu/AlphaLinux/Alpha64Decoder.h"
#include "Emu/AlphaLinux/AlphaOperation.h"

using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::AlphaLinux;
using namespace Onikiri::EmulatorUtility::Operation;
using namespace Onikiri::AlphaLinux::Operation;

//
// 命令の定義
//

namespace {



    // 各命令形式に対するオペコードを得るためのマスク (0のビットが引数)
    const u32 MASK_EXACT = 0xffffffff;  // 全bitが一致
    const u32 MASK_PAL  = 0xffffffff;   // PAL
    const u32 MASK_MEM = 0xfc000000;    // メモリ形式
    const u32 MASK_MEMF = 0xfc00ffff;   // 変位を機能コードとして用いるメモリ形式
    const u32 MASK_OPF = 0xfc00ffe0;    // 操作形式(浮動小数)
    const u32 MASK_OPI  = 0xfc001fe0;   // 操作形式(整数)
    const u32 MASK_BR  = 0xfc000000;    // 分岐形式
    const u32 MASK_JMP = 0xfc00c000;    // ジャンプ

    // PALコード
    const u32 PAL_HALT = 0x00000000;
    const u32 PAL_CALLSYS = 0x00000083;
    const u32 PAL_IMB = 0x00000086;
    const u32 PAL_RDUNIQ = 0x0000009e;
    const u32 PAL_WRUNIQ = 0x0000009f;
    const u32 PAL_GENTRAP = 0x000000aa;
}

#define OPCODE_PAL(c, f) (u32)((c) << 26 | (f))
#define OPCODE_MEM(c) (u32)((c) << 26)
#define OPCODE_MEMF(c, f) (u32)((c) << 26 | (f))
#define OPCODE_JMP(c, f) (u32)((c) << 26 | (f) << 14)
#define OPCODE_BR(c) (u32)((c) << 26)
// 機能形式 (第2ソースオペランド = Rb)
#define OPCODE_OPI(c, f) (u32)((c) << 26 | (f) << 5) 
// 機能形式 (第2ソースオペランド = リテラル)
#define OPCODE_OPIL(c, f) (u32)((c) << 26 | 1 << 12 | (f) << 5) 
#define OPCODE_OPF(c, f) (u32)((c) << 26 | (f) << 5) 


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

    const int T0 = Alpha64Info::REG_ADDRESS;
    const int FPC = Alpha64Info::REG_FPCR;
}

#define ALPHA_DSTOP(n) DstOperand<n>
#define ALPHA_SRCOP(n) SrcOperand<n>
#define ALPHA_SRCOPFLOAT(n) Cast< float, AsFP< double, SrcOperand<n> > >
#define ALPHA_SRCOPDOUBLE(n) AsFP< double, SrcOperand<n> >

#define D0 ALPHA_DSTOP(0)
#define D1 ALPHA_DSTOP(1)
#define S0 ALPHA_SRCOP(0)
#define S1 ALPHA_SRCOP(1)
#define S2 ALPHA_SRCOP(2)
#define S3 ALPHA_SRCOP(3)
#define SF0 ALPHA_SRCOPFLOAT(0)
#define SF1 ALPHA_SRCOPFLOAT(1)
#define SF2 ALPHA_SRCOPFLOAT(2)
#define SF3 ALPHA_SRCOPFLOAT(3)
#define SD0 ALPHA_SRCOPDOUBLE(0)
#define SD1 ALPHA_SRCOPDOUBLE(1)
#define SD2 ALPHA_SRCOPDOUBLE(2)
#define SD3 ALPHA_SRCOPDOUBLE(3)

// 00000000 PAL 00 = halt
// 2ffe0000 ldq_u r31, r30(0) = unop
// 471f041f mslql r31, r31, r31 = nop

// no trap implemented

// 投機的にフェッチされたときにはエラーにせず，実行されたときにエラーにする
// syscallにすることにより，直前までの命令が完了してから実行される (実行は投機的でない)
Alpha64Converter::OpDef Alpha64Converter::m_OpDefUnknown = 
    {"unknown", MASK_EXACT, 0,  1, {{OpClassCode::UNDEF,    {-1, -1}, {I0, -1, -1, -1}, Alpha64Converter::AlphaUnknownOperation}}};


// branchは，OpInfo 列の最後じゃないとだめ
Alpha64Converter::OpDef Alpha64Converter::m_OpDefsBase[] = 
{
    //
    // pal
    //
    {"halt",    MASK_PAL, OPCODE_PAL(0x00, PAL_HALT),       1, {{OpClassCode::syscall,  {-1, -1}, {-1, -1, -1, -1}, AlphaPALHalt}}},
    {"imb",     MASK_PAL, OPCODE_PAL(0x00, PAL_IMB),        1, {{OpClassCode::syscall,  {-1, -1}, {-1, -1, -1, -1}, AlphaPALIMB}}},
    {"rduniq",  MASK_PAL, OPCODE_PAL(0x00, PAL_RDUNIQ),     1, {{OpClassCode::syscall,  { 0, -1}, {-1, -1, -1, -1}, AlphaPALRdUniq}}},
    {"wruniq",  MASK_PAL, OPCODE_PAL(0x00, PAL_WRUNIQ),     1, {{OpClassCode::syscall,  {-1, -1}, {16, -1, -1, -1}, AlphaPALWrUniq}}},
    {"gentrap", MASK_PAL, OPCODE_PAL(0x00, PAL_GENTRAP),    1, {{OpClassCode::syscall,  {-1, -1}, {-1, -1, -1, -1}, AlphaPALGenTrap}}},

    // system call
    {"callsys", MASK_PAL, OPCODE_PAL(0x00, PAL_CALLSYS),    2, {
        {OpClassCode::syscall,  {-1, -1}, { 0, 16, 17, -1}, AlphaSyscallSetArg},
        {OpClassCode::syscall_branch,   { 0, 19}, {18, 19, 20, -1}, AlphaSyscallCore},
    }},

    //
    // special instructions
    //
    {"nop",     MASK_EXACT, 0x47ff041f, 1, {{OpClassCode::iNOP, {-1, -1}, {-1, -1, -1, -1}, NoOperation}}},
    {"fnop",    MASK_EXACT, 0x5fff041f, 1, {{OpClassCode::fNOP, {-1, -1}, {-1, -1, -1, -1}, NoOperation}}},
    // unopはstack topを読み出すのでNOPにしない方がよい？
    {"unop",    MASK_EXACT, 0x2ffe0000, 1, {{OpClassCode::iNOP, {-1, -1}, {-1, -1, -1, -1}, NoOperation}}},


    //
    // int
    //
    {"implver", MASK_OPI, OPCODE_OPIL(0x11, 0x6c),  1, {{OpClassCode::syscall,  {R0, -1}, {-1, -1, -1, -1}, UndefinedOperation}}},

    {"amask",   MASK_OPI, OPCODE_OPI(0x11, 0x61),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, UndefinedOperation}}},
    {"amask",   MASK_OPI, OPCODE_OPIL(0x11, 0x61),  1, {{OpClassCode::iALU, {R0, -1}, {R1, -1, -1, -1}, UndefinedOperation}}},

    {"addl",    MASK_OPI, OPCODE_OPI(0x10, 0x00),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, SetSext< D0, IntAdd<u32, S0, S1> >}}},
    {"addl",    MASK_OPI, OPCODE_OPIL(0x10, 0x00),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, SetSext< D0, IntAdd<u32, S0, S1> >}}},

    {"subl",    MASK_OPI, OPCODE_OPI(0x10, 0x09),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, SetSext< D0, IntSub<u32, S0, S1> >}}},
    {"subl",    MASK_OPI, OPCODE_OPIL(0x10, 0x09),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, SetSext< D0, IntSub<u32, S0, S1> >}}},

    {"addq",    MASK_OPI, OPCODE_OPI(0x10, 0x20),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, IntAdd<u64, S0, S1> >}}},
    {"addq",    MASK_OPI, OPCODE_OPIL(0x10, 0x20),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, IntAdd<u64, S0, S1> >}}},

    {"subq",    MASK_OPI, OPCODE_OPI(0x10, 0x29),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, IntSub<u64, S0, S1> >}}},
    {"subq",    MASK_OPI, OPCODE_OPIL(0x10, 0x29),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, IntSub<u64, S0, S1> >}}},

    {"cmpeq",   MASK_OPI, OPCODE_OPI(0x10, 0x2d),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaCompare< S0, S1, IntCondEqual<u64> > >}}},
    {"cmpeq",   MASK_OPI, OPCODE_OPIL(0x10, 0x2d),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaCompare< S0, S1, IntCondEqual<u64> > >}}},

    {"cmplt",   MASK_OPI, OPCODE_OPI(0x10, 0x4d),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaCompare< S0, S1, IntCondLessSigned<u64> > >}}},
    {"cmplt",   MASK_OPI, OPCODE_OPIL(0x10, 0x4d),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaCompare< S0, S1, IntCondLessSigned<u64> > >}}},

    {"cmple",   MASK_OPI, OPCODE_OPI(0x10, 0x6d),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaCompare< S0, S1, IntCondLessEqualSigned<u64> > >}}},
    {"cmple",   MASK_OPI, OPCODE_OPIL(0x10, 0x6d),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaCompare< S0, S1, IntCondLessEqualSigned<u64> > >}}},

    {"cmpult",  MASK_OPI, OPCODE_OPI(0x10, 0x1d),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaCompare< S0, S1, IntCondLessUnsigned<u64> > >}}},
    {"cmpult",  MASK_OPI, OPCODE_OPIL(0x10, 0x1d),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaCompare< S0, S1, IntCondLessUnsigned<u64> > >}}},

    {"cmpule",  MASK_OPI, OPCODE_OPI(0x10, 0x3d),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaCompare< S0, S1, IntCondLessEqualUnsigned<u64> > >}}},
    {"cmpule",  MASK_OPI, OPCODE_OPIL(0x10, 0x3d),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaCompare< S0, S1, IntCondLessEqualUnsigned<u64> > >}}},

    {"s4addl",  MASK_OPI, OPCODE_OPI(0x10, 0x02),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, SetSext< D0, IntScaledAdd< u32, 2, S0, S1> >}}},
    {"s4addl",  MASK_OPI, OPCODE_OPIL(0x10, 0x02),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, SetSext< D0, IntScaledAdd< u32, 2, S0, S1> >}}},
    
    {"s4addq",  MASK_OPI, OPCODE_OPI(0x10, 0x22),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, IntScaledAdd< u64, 2, S0, S1> >}}},
    {"s4addq",  MASK_OPI, OPCODE_OPIL(0x10, 0x22),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, IntScaledAdd< u64, 2, S0, S1> >}}},

    {"s4subl",  MASK_OPI, OPCODE_OPI(0x10, 0x0b),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, SetSext< D0, IntScaledSub< u32, 2, S0, S1> >}}},
    {"s4subl",  MASK_OPI, OPCODE_OPIL(0x10, 0x0b),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, SetSext< D0, IntScaledSub< u32, 2, S0, S1> >}}},
    
    {"s4subq",  MASK_OPI, OPCODE_OPI(0x10, 0x2b),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, IntScaledSub< u64, 2, S0, S1> >}}},
    {"s4subq",  MASK_OPI, OPCODE_OPIL(0x10, 0x2b),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, IntScaledSub< u64, 2, S0, S1> >}}},

    {"s8addl",  MASK_OPI, OPCODE_OPI(0x10, 0x12),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, SetSext< D0, IntScaledAdd< u32, 3, S0, S1> >}}},
    {"s8addl",  MASK_OPI, OPCODE_OPIL(0x10, 0x12),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, SetSext< D0, IntScaledAdd< u32, 3, S0, S1> >}}},
    
    {"s8addq",  MASK_OPI, OPCODE_OPI(0x10, 0x32),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, IntScaledAdd< u64, 3, S0, S1> >}}},
    {"s8addq",  MASK_OPI, OPCODE_OPIL(0x10, 0x32),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, IntScaledAdd< u64, 3, S0, S1> >}}},

    {"s8subl",  MASK_OPI, OPCODE_OPI(0x10, 0x1b),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, SetSext< D0, IntScaledSub< u32, 3, S0, S1> >}}},
    {"s8subl",  MASK_OPI, OPCODE_OPIL(0x10, 0x1b),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, SetSext< D0, IntScaledSub< u32, 3, S0, S1> >}}},
    
    {"s8subq",  MASK_OPI, OPCODE_OPI(0x10, 0x3b),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, IntScaledSub< u64, 3, S0, S1> >}}},
    {"s8subq",  MASK_OPI, OPCODE_OPIL(0x10, 0x3b),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, IntScaledSub< u64, 3, S0, S1> >}}},

    // mullはsigned mulでなければならないような気がするが現状SPEC全て動いているので触らないでおく
    // 問題があれば IntMul<s32, S0, S1> に変更する．mulqも同じ．
    {"mull",    MASK_OPI, OPCODE_OPI(0x13, 0x00),   1, {{OpClassCode::iMUL, {R0, -1}, {R1, R2, -1, -1}, SetSext< D0, IntMul<u32, S0, S1> >}}},
    {"mull",    MASK_OPI, OPCODE_OPIL(0x13, 0x00),  1, {{OpClassCode::iMUL, {R0, -1}, {R1, I0, -1, -1}, SetSext< D0, IntMul<u32, S0, S1> >}}},

    {"mulq",    MASK_OPI, OPCODE_OPI(0x13, 0x20),   1, {{OpClassCode::iMUL, {R0, -1}, {R1, R2, -1, -1}, Set< D0, IntMul<u64, S0, S1> >}}},
    {"mulq",    MASK_OPI, OPCODE_OPIL(0x13, 0x20),  1, {{OpClassCode::iMUL, {R0, -1}, {R1, I0, -1, -1}, Set< D0, IntMul<u64, S0, S1> >}}},

    {"umulh",   MASK_OPI, OPCODE_OPI(0x13, 0x30),   1, {{OpClassCode::iMUL, {R0, -1}, {R1, R2, -1, -1}, Set< D0, IntUMulh64< S0, S1 > >}}},
    {"umulh",   MASK_OPI, OPCODE_OPIL(0x13, 0x30),  1, {{OpClassCode::iMUL, {R0, -1}, {R1, I0, -1, -1}, Set< D0, IntUMulh64< S0, S1 > >}}},

    // compare and move
    {"cmoveq",  MASK_OPI, OPCODE_OPI(0x11, 0x24),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R0, R2, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, IntCondEqual<u64> >, S2, S1 > >}}},
    {"cmoveq",  MASK_OPI, OPCODE_OPIL(0x11, 0x24),  1, {{OpClassCode::iALU, {R0, -1}, {R1, R0, I0, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, IntCondEqual<u64> >, S2, S1 > >}}},

    {"cmovlt",  MASK_OPI, OPCODE_OPI(0x11, 0x44),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R0, R2, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, IntCondLessSigned<u64> >, S2, S1 > >}}},
    {"cmovlt",  MASK_OPI, OPCODE_OPIL(0x11, 0x44),  1, {{OpClassCode::iALU, {R0, -1}, {R1, R0, I0, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, IntCondLessSigned<u64> >, S2, S1 > >}}},

    {"cmovle",  MASK_OPI, OPCODE_OPI(0x11, 0x64),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R0, R2, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, IntCondLessEqualSigned<u64> >, S2, S1 > >}}},
    {"cmovle",  MASK_OPI, OPCODE_OPIL(0x11, 0x64),  1, {{OpClassCode::iALU, {R0, -1}, {R1, R0, I0, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, IntCondLessEqualSigned<u64> >, S2, S1 > >}}},

    {"cmovne",  MASK_OPI, OPCODE_OPI(0x11, 0x26),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R0, R2, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, IntCondNotEqual<u64> >, S2, S1 > >}}},
    {"cmovne",  MASK_OPI, OPCODE_OPIL(0x11, 0x26),  1, {{OpClassCode::iALU, {R0, -1}, {R1, R0, I0, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, IntCondNotEqual<u64> >, S2, S1 > >}}},

    {"cmovge",  MASK_OPI, OPCODE_OPI(0x11, 0x46),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R0, R2, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, IntCondGreaterEqualSigned<u64> >, S2, S1 > >}}},
    {"cmovge",  MASK_OPI, OPCODE_OPIL(0x11, 0x46),  1, {{OpClassCode::iALU, {R0, -1}, {R1, R0, I0, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, IntCondGreaterEqualSigned<u64> >, S2, S1 > >}}},

    {"cmovgt",  MASK_OPI, OPCODE_OPI(0x11, 0x66),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R0, R2, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, IntCondGreaterSigned<u64> >, S2, S1 > >}}},
    {"cmovgt",  MASK_OPI, OPCODE_OPIL(0x11, 0x66),  1, {{OpClassCode::iALU, {R0, -1}, {R1, R0, I0, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, IntCondGreaterSigned<u64> >, S2, S1 > >}}},

    {"cmovlbs", MASK_OPI, OPCODE_OPI(0x11, 0x14),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R0, R2, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, IntCondNotEqualNthBit<u64,0> >, S2, S1 > >}}},
    {"cmovlbs", MASK_OPI, OPCODE_OPIL(0x11, 0x14),  1, {{OpClassCode::iALU, {R0, -1}, {R1, R0, I0, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, IntCondNotEqualNthBit<u64,0> >, S2, S1 > >}}},

    {"cmovlbc", MASK_OPI, OPCODE_OPI(0x11, 0x16),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R0, R2, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, IntCondEqualNthBit<u64,0> >, S2, S1 > >}}},
    {"cmovlbc", MASK_OPI, OPCODE_OPIL(0x11, 0x16),  1, {{OpClassCode::iALU, {R0, -1}, {R1, R0, I0, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, IntCondEqualNthBit<u64,0> >, S2, S1 > >}}},

    // logical
    {"and", MASK_OPI, OPCODE_OPI(0x11, 0x00),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, BitAnd< u64, S0, S1 > >}}},
    {"and", MASK_OPI, OPCODE_OPIL(0x11, 0x00),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, BitAnd< u64, S0, S1 > >}}},

    {"bis", MASK_OPI, OPCODE_OPI(0x11, 0x20),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, BitOr< u64, S0, S1 > >}}},
    {"bis", MASK_OPI, OPCODE_OPIL(0x11, 0x20),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, BitOr< u64, S0, S1 > >}}},

    {"xor", MASK_OPI, OPCODE_OPI(0x11, 0x40),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, BitXor< u64, S0, S1 > >}}},
    {"xor", MASK_OPI, OPCODE_OPIL(0x11, 0x40),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, BitXor< u64, S0, S1 > >}}},

    {"bic", MASK_OPI, OPCODE_OPI(0x11, 0x08),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, BitAndNot< u64, S0, S1 > >}}},
    {"bic", MASK_OPI, OPCODE_OPIL(0x11, 0x08),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, BitAndNot< u64, S0, S1 > >}}},

    {"ornot",   MASK_OPI, OPCODE_OPI(0x11, 0x28),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, BitOrNot< u64, S0, S1 > >}}},
    {"ornot",   MASK_OPI, OPCODE_OPIL(0x11, 0x28),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, BitOrNot< u64, S0, S1 > >}}},

    {"eqv", MASK_OPI, OPCODE_OPI(0x11, 0x48),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, BitXorNot< u64, S0, S1 > >}}},
    {"eqv", MASK_OPI, OPCODE_OPIL(0x11, 0x48),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, BitXorNot< u64, S0, S1 > >}}},

    {"sll", MASK_OPI, OPCODE_OPI(0x12, 0x39),   1, {{OpClassCode::iSFT, {R0, -1}, {R1, R2, -1, -1}, Set< D0, LShiftL< u64, S0, S1, 0x3f > >}}},
    {"sll", MASK_OPI, OPCODE_OPIL(0x12, 0x39),  1, {{OpClassCode::iSFT, {R0, -1}, {R1, I0, -1, -1}, Set< D0, LShiftL< u64, S0, S1, 0x3f > >}}},

    {"sra", MASK_OPI, OPCODE_OPI(0x12, 0x3c),   1, {{OpClassCode::iSFT, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AShiftR< u64, S0, S1, 0x3f > >}}},
    {"sra", MASK_OPI, OPCODE_OPIL(0x12, 0x3c),  1, {{OpClassCode::iSFT, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AShiftR< u64, S0, S1, 0x3f > >}}},

    {"srl", MASK_OPI, OPCODE_OPI(0x12, 0x34),   1, {{OpClassCode::iSFT, {R0, -1}, {R1, R2, -1, -1}, Set< D0, LShiftR< u64, S0, S1, 0x3f > >}}},
    {"srl", MASK_OPI, OPCODE_OPIL(0x12, 0x34),  1, {{OpClassCode::iSFT, {R0, -1}, {R1, I0, -1, -1}, Set< D0, LShiftR< u64, S0, S1, 0x3f > >}}},

    // byte-wise
    {"cmpbge",  MASK_OPI, OPCODE_OPI(0x10, 0x0f),   1, {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaCmpBge< S0, S1 > >}}},
    {"cmpbge",  MASK_OPI, OPCODE_OPIL(0x10, 0x0f),  1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaCmpBge< S0, S1 > >}}},

    {"extbl",   MASK_OPI, OPCODE_OPI(0x12, 0x06),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaExtxl<  u8, S0, S1 > >}}},
    {"extbl",   MASK_OPI, OPCODE_OPIL(0x12, 0x06),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaExtxl<  u8, S0, S1 > >}}},

    {"extwl",   MASK_OPI, OPCODE_OPI(0x12, 0x16),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaExtxl< u16, S0, S1 > >}}},
    {"extwl",   MASK_OPI, OPCODE_OPIL(0x12, 0x16),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaExtxl< u16, S0, S1 > >}}},

    {"extll",   MASK_OPI, OPCODE_OPI(0x12, 0x26),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaExtxl< u32, S0, S1 > >}}},
    {"extll",   MASK_OPI, OPCODE_OPIL(0x12, 0x26),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaExtxl< u32, S0, S1 > >}}},

    {"extql",   MASK_OPI, OPCODE_OPI(0x12, 0x36),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaExtxl< u64, S0, S1 > >}}},
    {"extql",   MASK_OPI, OPCODE_OPIL(0x12, 0x36),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaExtxl< u64, S0, S1 > >}}},

    {"extwh",   MASK_OPI, OPCODE_OPI(0x12, 0x5a),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaExtxh< u16, S0, S1 > >}}},
    {"extwh",   MASK_OPI, OPCODE_OPIL(0x12, 0x5a),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaExtxh< u16, S0, S1 > >}}},

    {"extlh",   MASK_OPI, OPCODE_OPI(0x12, 0x6a),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaExtxh< u32, S0, S1 > >}}},
    {"extlh",   MASK_OPI, OPCODE_OPIL(0x12, 0x6a),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaExtxh< u32, S0, S1 > >}}},

    {"extqh",   MASK_OPI, OPCODE_OPI(0x12, 0x7a),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaExtxh< u64, S0, S1 > >}}},
    {"extqh",   MASK_OPI, OPCODE_OPIL(0x12, 0x7a),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaExtxh< u64, S0, S1 > >}}},

    {"insbl",   MASK_OPI, OPCODE_OPI(0x12, 0x0b),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaInsxl<  u8, S0, S1 > >}}},
    {"insbl",   MASK_OPI, OPCODE_OPIL(0x12, 0x0b),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaInsxl<  u8, S0, S1 > >}}},

    {"inswl",   MASK_OPI, OPCODE_OPI(0x12, 0x1b),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaInsxl< u16, S0, S1 > >}}},
    {"inswl",   MASK_OPI, OPCODE_OPIL(0x12, 0x1b),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaInsxl< u16, S0, S1 > >}}},

    {"insll",   MASK_OPI, OPCODE_OPI(0x12, 0x2b),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaInsxl< u32, S0, S1 > >}}},
    {"insll",   MASK_OPI, OPCODE_OPIL(0x12, 0x2b),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaInsxl< u32, S0, S1 > >}}},

    {"insql",   MASK_OPI, OPCODE_OPI(0x12, 0x3b),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaInsxl< u64, S0, S1 > >}}},
    {"insql",   MASK_OPI, OPCODE_OPIL(0x12, 0x3b),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaInsxl< u64, S0, S1 > >}}},

    {"inswh",   MASK_OPI, OPCODE_OPI(0x12, 0x57),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaInsxh< u16, S0, S1 > >}}},
    {"inswh",   MASK_OPI, OPCODE_OPIL(0x12, 0x57),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaInsxh< u16, S0, S1 > >}}},

    {"inslh",   MASK_OPI, OPCODE_OPI(0x12, 0x67),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaInsxh< u32, S0, S1 > >}}},
    {"inslh",   MASK_OPI, OPCODE_OPIL(0x12, 0x67),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaInsxh< u32, S0, S1 > >}}},

    {"insqh",   MASK_OPI, OPCODE_OPI(0x12, 0x77),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaInsxh< u64, S0, S1 > >}}},
    {"insqh",   MASK_OPI, OPCODE_OPIL(0x12, 0x77),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaInsxh< u64, S0, S1 > >}}},

    {"mskbl",   MASK_OPI, OPCODE_OPI(0x12, 0x02),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaMskxl<  u8, S0, S1 > >}}},
    {"mskbl",   MASK_OPI, OPCODE_OPIL(0x12, 0x02),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaMskxl<  u8, S0, S1 > >}}},

    {"mskwl",   MASK_OPI, OPCODE_OPI(0x12, 0x12),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaMskxl< u16, S0, S1 > >}}},
    {"mskwl",   MASK_OPI, OPCODE_OPIL(0x12, 0x12),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaMskxl< u16, S0, S1 > >}}},

    {"mskll",   MASK_OPI, OPCODE_OPI(0x12, 0x22),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaMskxl< u32, S0, S1 > >}}},
    {"mskll",   MASK_OPI, OPCODE_OPIL(0x12, 0x22),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaMskxl< u32, S0, S1 > >}}},

    {"mskql",   MASK_OPI, OPCODE_OPI(0x12, 0x32),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaMskxl< u64, S0, S1 > >}}},
    {"mskql",   MASK_OPI, OPCODE_OPIL(0x12, 0x32),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaMskxl< u64, S0, S1 > >}}},

    {"mskwh",   MASK_OPI, OPCODE_OPI(0x12, 0x52),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaMskxh< u16, S0, S1 > >}}},
    {"mskwh",   MASK_OPI, OPCODE_OPIL(0x12, 0x52),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaMskxh< u16, S0, S1 > >}}},

    {"msklh",   MASK_OPI, OPCODE_OPI(0x12, 0x62),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaMskxh< u32, S0, S1 > >}}},
    {"msklh",   MASK_OPI, OPCODE_OPIL(0x12, 0x62),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaMskxh< u32, S0, S1 > >}}},

    {"mskqh",   MASK_OPI, OPCODE_OPI(0x12, 0x72),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaMskxh< u64, S0, S1 > >}}},
    {"mskqh",   MASK_OPI, OPCODE_OPIL(0x12, 0x72),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaMskxh< u64, S0, S1 > >}}},

    {"zap", MASK_OPI, OPCODE_OPI(0x12, 0x30),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaZap< S0, S1 > >}}},
    {"zap", MASK_OPI, OPCODE_OPIL(0x12, 0x30),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaZap< S0, S1 > >}}},

    {"zapnot",  MASK_OPI, OPCODE_OPI(0x12, 0x31),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaZapNot< S0, S1 > >}}},
    {"zapnot",  MASK_OPI, OPCODE_OPIL(0x12, 0x31),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaZapNot< S0, S1 > >}}},

    // BWX
    {"sextb",   MASK_OPI, OPCODE_OPI(0x1c, 0x00),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaSextb< S1 > >}}},
    {"sextb",   MASK_OPI, OPCODE_OPIL(0x1c, 0x00),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaSextb< S1 > >}}},
    {"sextw",   MASK_OPI, OPCODE_OPI(0x1c, 0x01),   1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaSextw< S1 > >}}},
    {"sextw",   MASK_OPI, OPCODE_OPIL(0x1c, 0x01),  1, {{OpClassCode::iBYTE,    {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaSextw< S1 > >}}},

    //
    // fp
    //

    // 丸めモード
    // --: 標準 (unbiased rounding to nearest)
    // /c: 切り捨て
    // /m: 負の無限大へ
    // /d: 動的 (FP Control Register)

    // FPCR[59-58]
    //  - 00 : chopped
    //  - 01 : minus inf.
    //  - 10 : normal
    //  - 11 : plus inf.


    // 例外モード
    // /s: ソフトウェア完了
    // /u: アンダーフロー有効
    // /i: 不正確結果有効

    // 整数への変換時例外モード
    // /v: 整数オーバーフロー有効

    // <TODO> /d : 動的モードへの対応
    // ↓こんな感じで可能.
    // {"adds/sud", MASK_OPF, OPCODE_OPF(0x16, 0x5c0),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2,FPC, -1}, SetFP< D0, Cast< double, FPAdd< float, SF0, SF1, AlphaRoundModeFromFPCR<S2> > >}}},

    {"adds"    ,    MASK_OPF, OPCODE_OPF(0x16, 0x080),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPAdd< float, SF0, SF1, IntConst<int, FE_TONEAREST> > > >}}},
    {"adds/su" ,    MASK_OPF, OPCODE_OPF(0x16, 0x580),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPAdd< float, SF0, SF1, IntConst<int, FE_TONEAREST> > > >}}},
    {"adds/suc",    MASK_OPF, OPCODE_OPF(0x16, 0x500),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPAdd< float, SF0, SF1, IntConst<int, FE_TOWARDZERO> > > >}}},
    {"adds/sum",    MASK_OPF, OPCODE_OPF(0x16, 0x540),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPAdd< float, SF0, SF1, IntConst<int, FE_DOWNWARD> > > >}}},
    {"adds/sud",    MASK_OPF, OPCODE_OPF(0x16, 0x5c0),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2,FPC, -1}, SetFP< D0, Cast< double, FPAdd< float, SF0, SF1, AlphaRoundModeFromFPCR<S2>  > > >}}},
    {"addt"    ,    MASK_OPF, OPCODE_OPF(0x16, 0x0a0),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPAdd< double, SD0, SD1, IntConst<int, FE_TONEAREST> > >}}},
    {"addt/su" ,    MASK_OPF, OPCODE_OPF(0x16, 0x5a0),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPAdd< double, SD0, SD1, IntConst<int, FE_TONEAREST> > >}}},
    {"addt/suc",    MASK_OPF, OPCODE_OPF(0x16, 0x520),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPAdd< double, SD0, SD1, IntConst<int, FE_TOWARDZERO> > >}}},
    {"addt/sum",    MASK_OPF, OPCODE_OPF(0x16, 0x560),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPAdd< double, SD0, SD1, IntConst<int, FE_DOWNWARD> > >}}},
    {"addt/sud",    MASK_OPF, OPCODE_OPF(0x16, 0x5e0),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2,FPC, -1}, SetFP< D0, FPAdd< double, SD0, SD1, AlphaRoundModeFromFPCR<S2>  > >}}},
    {"subs"    ,    MASK_OPF, OPCODE_OPF(0x16, 0x081),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPSub< float, SF0, SF1, IntConst<int, FE_TONEAREST> > > >}}},
    {"subs/su" ,    MASK_OPF, OPCODE_OPF(0x16, 0x581),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPSub< float, SF0, SF1, IntConst<int, FE_TONEAREST> > > >}}},
    {"subs/suc",    MASK_OPF, OPCODE_OPF(0x16, 0x501),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPSub< float, SF0, SF1, IntConst<int, FE_TOWARDZERO> > > >}}},
    {"subs/sum",    MASK_OPF, OPCODE_OPF(0x16, 0x541),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPSub< float, SF0, SF1, IntConst<int, FE_DOWNWARD> > > >}}},
    {"subs/sud",    MASK_OPF, OPCODE_OPF(0x16, 0x5c1),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2,FPC, -1}, SetFP< D0, Cast< double, FPSub< float, SF0, SF1 > > >}}},
    {"subt"    ,    MASK_OPF, OPCODE_OPF(0x16, 0x0a1),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPSub< double, SD0, SD1, IntConst<int, FE_TONEAREST> > >}}},
    {"subt/su" ,    MASK_OPF, OPCODE_OPF(0x16, 0x5a1),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPSub< double, SD0, SD1, IntConst<int, FE_TONEAREST> > >}}},
    {"subt/suc",    MASK_OPF, OPCODE_OPF(0x16, 0x521),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPSub< double, SD0, SD1, IntConst<int, FE_TOWARDZERO> > >}}},
    {"subt/sum",    MASK_OPF, OPCODE_OPF(0x16, 0x561),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPSub< double, SD0, SD1, IntConst<int, FE_DOWNWARD>  > >}}},
    {"subt/sud",    MASK_OPF, OPCODE_OPF(0x16, 0x5e1),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2,FPC, -1}, SetFP< D0, FPSub< double, SD0, SD1, AlphaRoundModeFromFPCR<S2>  > >}}},
    {"muls"    ,    MASK_OPF, OPCODE_OPF(0x16, 0x082),  1, {{OpClassCode::fMUL, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPMul< float, SF0, SF1, IntConst<int, FE_TONEAREST> > > >}}},
    {"muls/su" ,    MASK_OPF, OPCODE_OPF(0x16, 0x582),  1, {{OpClassCode::fMUL, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPMul< float, SF0, SF1, IntConst<int, FE_TONEAREST> > > >}}},
    {"muls/suc" ,   MASK_OPF, OPCODE_OPF(0x16, 0x502),  1, {{OpClassCode::fMUL, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPMul< float, SF0, SF1, IntConst<int, FE_TOWARDZERO> > > >}}},
    {"muls/sum" ,   MASK_OPF, OPCODE_OPF(0x16, 0x542),  1, {{OpClassCode::fMUL, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPMul< float, SF0, SF1, IntConst<int, FE_DOWNWARD > > > >}}},
    {"muls/sud",    MASK_OPF, OPCODE_OPF(0x16, 0x5c2),  1, {{OpClassCode::fMUL, {R0, -1}, {R1, R2,FPC, -1}, SetFP< D0, Cast< double, FPMul< float, SF0, SF1, AlphaRoundModeFromFPCR<S2>  > > >}}},
    {"mult"    ,    MASK_OPF, OPCODE_OPF(0x16, 0x0a2),  1, {{OpClassCode::fMUL, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPMul< double, SD0, SD1, IntConst<int, FE_TONEAREST> > >}}},
    {"mult/c"  ,    MASK_OPF, OPCODE_OPF(0x16, 0x022),  1, {{OpClassCode::fMUL, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPMul< double, SD0, SD1, IntConst<int, FE_TOWARDZERO> > >}}},
    {"mult/su" ,    MASK_OPF, OPCODE_OPF(0x16, 0x5a2),  1, {{OpClassCode::fMUL, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPMul< double, SD0, SD1, IntConst<int, FE_TONEAREST> > >}}},
    {"mult/suc" ,   MASK_OPF, OPCODE_OPF(0x16, 0x522),  1, {{OpClassCode::fMUL, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPMul< double, SD0, SD1, IntConst<int, FE_TOWARDZERO> > >}}},
    {"mult/sum" ,   MASK_OPF, OPCODE_OPF(0x16, 0x562),  1, {{OpClassCode::fMUL, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPMul< double, SD0, SD1, IntConst<int, FE_DOWNWARD > > >}}},
    {"mult/sud",    MASK_OPF, OPCODE_OPF(0x16, 0x5e2),  1, {{OpClassCode::fMUL, {R0, -1}, {R1, R2,FPC, -1}, SetFP< D0, FPMul< double, SD0, SD1, AlphaRoundModeFromFPCR<S2>  > >}}},
    {"divs"    ,    MASK_OPF, OPCODE_OPF(0x16, 0x083),  1, {{OpClassCode::fDIV, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPDiv< float, SF0, SF1, IntConst<int, FE_TONEAREST> > > >}}},
    {"divs/c"  ,    MASK_OPF, OPCODE_OPF(0x16, 0x003),  1, {{OpClassCode::fDIV, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPDiv< float, SF0, SF1, IntConst<int, FE_TOWARDZERO> > > >}}},
    {"divs/su" ,    MASK_OPF, OPCODE_OPF(0x16, 0x583),  1, {{OpClassCode::fDIV, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPDiv< float, SF0, SF1, IntConst<int, FE_TONEAREST> > > >}}},  
    {"divs/suc" ,   MASK_OPF, OPCODE_OPF(0x16, 0x503),  1, {{OpClassCode::fDIV, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPDiv< float, SF0, SF1, IntConst<int, FE_TOWARDZERO> > > >}}}, 
    {"divs/sum" ,   MASK_OPF, OPCODE_OPF(0x16, 0x543),  1, {{OpClassCode::fDIV, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, Cast< double, FPDiv< float, SF0, SF1, IntConst<int, FE_DOWNWARD> > > >}}},   
    {"divs/sud",    MASK_OPF, OPCODE_OPF(0x16, 0x5c3),  1, {{OpClassCode::fDIV, {R0, -1}, {R1, R2,FPC, -1}, SetFP< D0, Cast< double, FPDiv< float, SF0, SF1, AlphaRoundModeFromFPCR<S2>  > > >}}},  
    {"divt"    ,    MASK_OPF, OPCODE_OPF(0x16, 0x0a3),  1, {{OpClassCode::fDIV, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPDiv< double, SD0, SD1, IntConst<int, FE_TONEAREST> > >}}},
    {"divt/c"  ,    MASK_OPF, OPCODE_OPF(0x16, 0x023),  1, {{OpClassCode::fDIV, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPDiv< double, SD0, SD1, IntConst<int, FE_TOWARDZERO> > >}}},
    {"divt/su" ,    MASK_OPF, OPCODE_OPF(0x16, 0x5a3),  1, {{OpClassCode::fDIV, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPDiv< double, SD0, SD1, IntConst<int, FE_TONEAREST> > >}}},
    {"divt/suc" ,   MASK_OPF, OPCODE_OPF(0x16, 0x523),  1, {{OpClassCode::fDIV, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPDiv< double, SD0, SD1, IntConst<int, FE_TOWARDZERO> > >}}},
    {"divt/sum" ,   MASK_OPF, OPCODE_OPF(0x16, 0x563),  1, {{OpClassCode::fDIV, {R0, -1}, {R1, R2, -1, -1}, SetFP< D0, FPDiv< double, SD0, SD1, IntConst<int, FE_DOWNWARD> > >}}},
    {"divt/sud",    MASK_OPF, OPCODE_OPF(0x16, 0x5e3),  1, {{OpClassCode::fDIV, {R0, -1}, {R1, R2,FPC, -1}, SetFP< D0, FPDiv< double, SD0, SD1, AlphaRoundModeFromFPCR<S2>  > >}}},

    {"cmpteq",  MASK_OPI, OPCODE_OPF(0x16, 0x0a5),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaTFloatCompare< S0, S1, FPCondEqual<f64> > >}}},
    {"cmptlt",  MASK_OPI, OPCODE_OPF(0x16, 0x0a6),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaTFloatCompare< S0, S1, FPCondLess<f64> > >}}},
    {"cmptle",  MASK_OPI, OPCODE_OPF(0x16, 0x0a7),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaTFloatCompare< S0, S1, FPCondLessEqual<f64> > >}}},
    {"cmptun",  MASK_OPI, OPCODE_OPF(0x16, 0x0a4),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaTFloatCompare< S0, S1, AlphaFPCondUnordered<f64> > >}}},
    {"cmptun/su",   MASK_OPI, OPCODE_OPF(0x16, 0x5a4),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaTFloatCompare< S0, S1, AlphaFPCondUnordered<f64> > >}}},

    {"cmpteq/su",   MASK_OPI, OPCODE_OPF(0x16, 0x5a5),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaTFloatCompare< S0, S1, FPCondEqual<f64> > >}}},
    {"cmptlt/su",   MASK_OPI, OPCODE_OPF(0x16, 0x5a6),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaTFloatCompare< S0, S1, FPCondLess<f64> > >}}},
    {"cmptle/su",   MASK_OPI, OPCODE_OPF(0x16, 0x5a7),  1, {{OpClassCode::fADD, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaTFloatCompare< S0, S1, FPCondLessEqual<f64> > >}}},

    {"cvtqs",   MASK_OPF, OPCODE_OPF(0x16, 0x0bc),  1, {{OpClassCode::fCONV,    {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaCvtQS< S0 > >}}},
    {"cvtqs/d", MASK_OPF, OPCODE_OPF(0x16, 0x0fc),  1, {{OpClassCode::fCONV,    {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaCvtQS< S0 > >}}},

    {"cvtqt",   MASK_OPF, OPCODE_OPF(0x16, 0x0be),  1, {{OpClassCode::fCONV,    {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaCvtQT< S0 > >}}},
    {"cvtqt/m", MASK_OPF, OPCODE_OPF(0x16, 0x07e),  1, {{OpClassCode::fCONV,    {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaCvtQT< S0 > >}}}, // m?
    {"cvtqt/d", MASK_OPF, OPCODE_OPF(0x16, 0x0fe),  1, {{OpClassCode::fCONV,    {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaCvtQT< S0 > >}}}, // d?
    {"cvtts",   MASK_OPF, OPCODE_OPF(0x16, 0x0ac),  1, {{OpClassCode::fCONV,    {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaCvtTS< S0 > >}}},
    {"cvtts/su",    MASK_OPF, OPCODE_OPF(0x16, 0x5ac),  1, {{OpClassCode::fCONV,    {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaCvtTS< S0 > >}}},
    {"cvtts/sud",   MASK_OPF, OPCODE_OPF(0x16, 0x5ec),  1, {{OpClassCode::fCONV,    {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaCvtTS< S0 > >}}}, // d?
    {"cvttq/c", MASK_OPF, OPCODE_OPF(0x16, 0x02f),  1, {{OpClassCode::fCONV,    {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaCvtTQ_c< S0 > >}}},
    {"cvttq/svc",   MASK_OPF, OPCODE_OPF(0x16, 0x52f),  1, {{OpClassCode::fCONV,    {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaCvtTQ_c< S0 > >}}},
    {"cvttq/svm",   MASK_OPF, OPCODE_OPF(0x16, 0x56f),  1, {{OpClassCode::fCONV,    {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaCvtTQ_m< S0 > >}}},   // floor
    {"cvttq/svd",   MASK_OPF, OPCODE_OPF(0x16, 0x5ef),  1, {{OpClassCode::fCONV,    {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaCvtTQ_m< S0 > >}}},   // d?


    // <TODO> ???
    {"cvtst",   MASK_OPF, OPCODE_OPF(0x16, 0x2ac),  1, {{OpClassCode::fCONV,    {R0, -1}, {R2, -1, -1, -1}, Set< D0, FPDoubleCopySign< S0, S0 > >}}},
    {"cvtst/s", MASK_OPF, OPCODE_OPF(0x16, 0x6ac),  1, {{OpClassCode::fCONV,    {R0, -1}, {R2, -1, -1, -1}, Set< D0, FPDoubleCopySign< S0, S0 > >}}},

    //
    {"cpys",    MASK_OPF, OPCODE_OPF(0x17, 0x020),  1, {{OpClassCode::fMOV, {R0, -1}, {R1, R2, -1, -1}, Set< D0, FPDoubleCopySign< S0, S1 > >}}},
    {"cpysn",   MASK_OPF, OPCODE_OPF(0x17, 0x021),  1, {{OpClassCode::fMOV, {R0, -1}, {R1, R2, -1, -1}, Set< D0, FPDoubleCopySignNeg< S0, S1 > >}}},
    {"cpyse",   MASK_OPF, OPCODE_OPF(0x17, 0x022),  1, {{OpClassCode::fMOV, {R0, -1}, {R1, R2, -1, -1}, Set< D0, FPDoubleCopySignExp< S0, S1 > >}}},

    {"cvtlq",   MASK_OPF, OPCODE_OPF(0x17, 0x010),  1, {{OpClassCode::fMOV, {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaCvtLQ< S0 > >}}},
    {"cvtql",   MASK_OPF, OPCODE_OPF(0x17, 0x030),  1, {{OpClassCode::fMOV, {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaCvtQL< S0 > >}}},
    {"cvtql/v", MASK_OPF, OPCODE_OPF(0x17, 0x130),  1, {{OpClassCode::fMOV, {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaCvtQL< S0 > >}}},
    {"cvtql/sv",    MASK_OPF, OPCODE_OPF(0x17, 0x530),  1, {{OpClassCode::fMOV, {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaCvtQL< S0 > >}}},

    {"mt_fpcr", MASK_OPF, OPCODE_OPF(0x17, 0x024),  1, {{OpClassCode::fMOV, {FPC,-1}, {R0, -1, -1, -1}, Set< D0, S0>}}},
    {"mf_fpcr", MASK_OPF, OPCODE_OPF(0x17, 0x025),  1, {{OpClassCode::fMOV, {R0, -1}, {FPC,-1, -1, -1}, Set< D0, S0>}}},

    {"fcmoveq", MASK_OPI, OPCODE_OPI(0x17, 0x02a),  1, {{OpClassCode::fMOV, {R0, -1}, {R1, R0, R2, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, FPCondEqual<f64> >, S2, S1 > >}}},
    {"fcmovne", MASK_OPI, OPCODE_OPI(0x17, 0x02b),  1, {{OpClassCode::fMOV, {R0, -1}, {R1, R0, R2, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, FPCondNotEqual<f64> >, S2, S1 > >}}},
    {"fcmovlt", MASK_OPI, OPCODE_OPI(0x17, 0x02c),  1, {{OpClassCode::fMOV, {R0, -1}, {R1, R0, R2, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, FPCondLess<f64> >, S2, S1 > >}}},
    {"fcmovge", MASK_OPI, OPCODE_OPI(0x17, 0x02d),  1, {{OpClassCode::fMOV, {R0, -1}, {R1, R0, R2, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, FPCondGreaterEqual<f64> >, S2, S1 > >}}},
    {"fcmovle", MASK_OPI, OPCODE_OPI(0x17, 0x02e),  1, {{OpClassCode::fMOV, {R0, -1}, {R1, R0, R2, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, FPCondLessEqual<f64> >, S2, S1 > >}}},
    {"fcmovgt", MASK_OPI, OPCODE_OPI(0x17, 0x02f),  1, {{OpClassCode::fMOV, {R0, -1}, {R1, R0, R2, -1}, Set< D0, Select< u64, Compare<S0, IntConst<u64, 0>, FPCondGreater<f64> >, S2, S1 > >}}},

    // FIX 命令
    {"sqrts",   MASK_OPF, OPCODE_OPF(0x14, 0x08b),  1, {{OpClassCode::fELEM, {R0, -1}, {R2, -1, -1, -1}, SetFP< D0, Cast< double, FPSqrt< float, SF0, IntConst<int, FE_TONEAREST> > > >}}},
    {"sqrts/su",    MASK_OPF, OPCODE_OPF(0x14, 0x58b),  1, {{OpClassCode::fELEM, {R0, -1}, {R2, -1, -1, -1}, SetFP< D0, Cast< double, FPSqrt< float, SF0, IntConst<int, FE_TONEAREST> > > >}}},
    {"sqrts/suc",   MASK_OPF, OPCODE_OPF(0x14, 0x50b),  1, {{OpClassCode::fELEM, {R0, -1}, {R2, -1, -1, -1}, SetFP< D0, Cast< double, FPSqrt< float, SF0, IntConst<int, FE_TOWARDZERO> > > >}}},
    {"sqrts/sum",   MASK_OPF, OPCODE_OPF(0x14, 0x54b),  1, {{OpClassCode::fELEM, {R0, -1}, {R2, -1, -1, -1}, SetFP< D0, Cast< double, FPSqrt< float, SF0, IntConst<int, FE_DOWNWARD> > > >}}},
    {"sqrts/sud",   MASK_OPF, OPCODE_OPF(0x14, 0x5cb),  1, {{OpClassCode::fELEM, {R0, -1}, {R2, FPC, -1, -1}, SetFP< D0, Cast< double, FPSqrt< float, SF0, AlphaRoundModeFromFPCR<S1> > > >}}},
    {"sqrtt",   MASK_OPF, OPCODE_OPF(0x14, 0x0ab),  1, {{OpClassCode::fELEM, {R0, -1}, {R2, -1, -1, -1}, SetFP< D0, FPSqrt< double, SD0, IntConst<int, FE_TONEAREST> > >}}},
    {"sqrtt/su",    MASK_OPF, OPCODE_OPF(0x14, 0x5ab),  1, {{OpClassCode::fELEM, {R0, -1}, {R2, -1, -1, -1}, SetFP< D0, FPSqrt< double, SD0, IntConst<int, FE_TONEAREST> > >}}},
    {"sqrtt/suc",   MASK_OPF, OPCODE_OPF(0x14, 0x52b),  1, {{OpClassCode::fELEM, {R0, -1}, {R2, -1, -1, -1}, SetFP< D0, FPSqrt< double, SD0, IntConst<int, FE_TOWARDZERO> > >}}},
    {"sqrtt/sum",   MASK_OPF, OPCODE_OPF(0x14, 0x56b),  1, {{OpClassCode::fELEM, {R0, -1}, {R2, -1, -1, -1}, SetFP< D0, FPSqrt< double, SD0, IntConst<int, FE_DOWNWARD> > >}}},
    {"sqrtt/sud",   MASK_OPF, OPCODE_OPF(0x14, 0x5eb),  1, {{OpClassCode::fELEM, {R0, -1}, {R2, FPC, -1, -1}, SetFP< D0, FPSqrt< double, SD0, AlphaRoundModeFromFPCR<S1> > >}}},
    {"ftois",   MASK_OPF, OPCODE_OPF(0x1c, 0x078),  1, {{OpClassCode::ifCONV, {R0, -1}, {R1, -1, -1, -1}, Set< D0, Cast< s64, AlphaFtois<S0> > >}}},
    {"ftoit",   MASK_OPF, OPCODE_OPF(0x1c, 0x070),  1, {{OpClassCode::ifCONV, {R0, -1}, {R1, -1, -1, -1}, Set< D0, S0 >}}},
    {"itofs",   MASK_OPF, OPCODE_OPF(0x14, 0x004),  1, {{OpClassCode::ifCONV, {R0, -1}, {R1, -1, -1, -1}, Set< D0, AlphaItofs<S0> >}}},
    {"itoft",   MASK_OPF, OPCODE_OPF(0x14, 0x024),  1, {{OpClassCode::ifCONV, {R0, -1}, {R1, -1, -1, -1}, Set< D0, S0 >}}},

    // CIX 命令
    {"ctlz",    MASK_OPI, OPCODE_OPI(0x1c, 0x32),   1,  {{OpClassCode::iALU, {R0, -1}, {R2, -1, -1, -1}, Set< D0, NumberOfLeadingZeros<u64, S0> >}}},
    {"ctpop",   MASK_OPI, OPCODE_OPI(0x1c, 0x30),   1,  {{OpClassCode::iALU, {R0, -1}, {R2, -1, -1, -1}, Set< D0, NumberOfPopulations<u64, S0> >}}},
    {"cttz",    MASK_OPI, OPCODE_OPI(0x1c, 0x33),   1,  {{OpClassCode::iALU, {R0, -1}, {R2, -1, -1, -1}, Set< D0, NumberOfTrailingZeros<u64, S0> >}}},

    // MAX 命令
    {"pklb",    MASK_OPI, OPCODE_OPI(0x1c, 0x37),   1,  {{OpClassCode::iALU, {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaPackLongWords<S0> >}}},
    {"pkwb",    MASK_OPI, OPCODE_OPI(0x1c, 0x36),   1,  {{OpClassCode::iALU, {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaPackWords<S0> >}}},
    {"unpkbl",  MASK_OPI, OPCODE_OPI(0x1c, 0x35),   1,  {{OpClassCode::iALU, {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaUnpackLongWords<S0> >}}},
    {"unpkbw",  MASK_OPI, OPCODE_OPI(0x1c, 0x34),   1,  {{OpClassCode::iALU, {R0, -1}, {R2, -1, -1, -1}, Set< D0, AlphaUnpackWords<S0> >}}},
    {"minub8",  MASK_OPI, OPCODE_OPI(0x1c, 0x3a),   1,  {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaVectorMin<u8,S0,S1> >}}},
    {"minub8i", MASK_OPI, OPCODE_OPIL(0x1c, 0x3a),  1,  {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaVectorMin<u8,S0,S1> >}}},
    {"minsb8",  MASK_OPI, OPCODE_OPI(0x1c, 0x38),   1,  {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaVectorMin<s8,S0,S1> >}}},
    {"minsb8i", MASK_OPI, OPCODE_OPIL(0x1c, 0x38),  1,  {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaVectorMin<s8,S0,S1> >}}},
    {"minuw4",  MASK_OPI, OPCODE_OPI(0x1c, 0x3b),   1,  {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaVectorMin<u16,S0,S1> >}}},
    {"minuw4i", MASK_OPI, OPCODE_OPIL(0x1c, 0x3b),  1,  {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaVectorMin<u16,S0,S1> >}}},
    {"minsw4",  MASK_OPI, OPCODE_OPI(0x1c, 0x39),   1,  {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaVectorMin<s16,S0,S1> >}}},
    {"minsw4i", MASK_OPI, OPCODE_OPIL(0x1c, 0x39),  1,  {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaVectorMin<s16,S0,S1> >}}},
    {"maxub8",  MASK_OPI, OPCODE_OPI(0x1c, 0x3c),   1,  {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaVectorMax<u8,S0,S1> >}}},
    {"maxub8i", MASK_OPI, OPCODE_OPIL(0x1c, 0x3c),  1,  {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaVectorMax<u8,S0,S1> >}}},
    {"maxsb8",  MASK_OPI, OPCODE_OPI(0x1c, 0x3e),   1,  {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaVectorMax<s8,S0,S1> >}}},
    {"maxsb8i", MASK_OPI, OPCODE_OPIL(0x1c, 0x3e),  1,  {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaVectorMax<s8,S0,S1> >}}},
    {"maxuw4",  MASK_OPI, OPCODE_OPI(0x1c, 0x3d),   1,  {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaVectorMax<u16,S0,S1> >}}},
    {"maxuw4i", MASK_OPI, OPCODE_OPIL(0x1c, 0x3d),  1,  {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaVectorMax<u16,S0,S1> >}}},
    {"maxsw4",  MASK_OPI, OPCODE_OPI(0x1c, 0x3f),   1,  {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaVectorMax<s16,S0,S1> >}}},
    {"maxsw4i", MASK_OPI, OPCODE_OPIL(0x1c, 0x3f),  1,  {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaVectorMax<s16,S0,S1> >}}},
    {"perr",    MASK_OPI, OPCODE_OPI(0x1c, 0x31),   1,  {{OpClassCode::iALU, {R0, -1}, {R1, R2, -1, -1}, Set< D0, AlphaPixelError<S0,S1> >}}},

    //
    // memory
    //
    // trap barrier とりあえずnoop
    {"trapb",   MASK_JMP, OPCODE_MEMF(0x18, 0x0000),    1, {{OpClassCode::iNOP, {-1, -1}, {-1, -1, -1, -1}, NoOperation}}},
    // シングルプロセッサなのでnoop
    {"mb",  MASK_JMP, OPCODE_MEMF(0x18, 0x4000),    1, {{OpClassCode::iNOP, {-1, -1}, {-1, -1, -1, -1}, NoOperation}}},
    {"wmb", MASK_JMP, OPCODE_MEMF(0x18, 0x4400),    1, {{OpClassCode::iNOP, {-1, -1}, {-1, -1, -1, -1}, NoOperation}}},


    // exception barrier
    {"excb",    MASK_JMP, OPCODE_MEMF(0x18, 0x0400),    1, {{OpClassCode::iNOP, {-1, -1}, {-1, -1, -1, -1}, NoOperation}}},
    // read processor cycle counter to Ra
    {"rpcc",    MASK_JMP, OPCODE_MEMF(0x18, 0xc000),    1, {{OpClassCode::iMOV, {-1, -1}, {-1, -1, -1, -1}, NoOperation}}},


    {"jmp", MASK_JMP, OPCODE_JMP(0x1a, 0x0),    1, {{OpClassCode::iJUMP,    {R0, -1}, {R1, -1, -1, -1}, CallAbsUncond< D0, S0 >}}},
    {"jsr", MASK_JMP, OPCODE_JMP(0x1a, 0x1),    1, {{OpClassCode::CALL_JUMP,    {R0, -1}, {R1, -1, -1, -1}, CallAbsUncond< D0, S0 >}}},
    {"ret", MASK_JMP, OPCODE_JMP(0x1a, 0x2),    1, {{OpClassCode::RET,  {R0, -1}, {R1, -1, -1, -1}, CallAbsUncond< D0, S0 >}}},
//  {"jsr_coroutine",   MASK_JMP, OPCODE_JMP(0x1a, 0x3),    1, {{OpClassCode::iBU,  {R0, -1}, {R1, -1, -1, -1}, alpha_jsr_coroutine< D0, S0 >}}},


    //
    // memory
    //
    {"lda",     MASK_MEM, OPCODE_MEM(0x08), 1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >}}},
    {"ldah",    MASK_MEM, OPCODE_MEM(0x09), 1, {{OpClassCode::iALU, {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaLdah< S0, S1 > >}}},


    //
    // branch
    //
    {"br",      MASK_BR, OPCODE_BR(0x30),   1, {{OpClassCode::iBU,  {R0, -1}, {I0, -1, -1, -1}, CallRelUncond< D0, S0 >}}},
    {"fbeq",    MASK_BR, OPCODE_BR(0x31),   1, {{OpClassCode::fBC,  {-1, -1}, {R1, I0, -1, -1}, BranchRelCond< S1, Compare< S0, IntConst<u64, 0>, FPCondEqual<f64> > >}}},
    {"fblt",    MASK_BR, OPCODE_BR(0x32),   1, {{OpClassCode::fBC,  {-1, -1}, {R1, I0, -1, -1}, BranchRelCond< S1, Compare< S0, IntConst<u64, 0>, FPCondLess<f64> > >}}},
    {"fble",    MASK_BR, OPCODE_BR(0x33),   1, {{OpClassCode::fBC,  {-1, -1}, {R1, I0, -1, -1}, BranchRelCond< S1, Compare< S0, IntConst<u64, 0>, FPCondLessEqual<f64> > >}}},
    {"bsr",     MASK_BR, OPCODE_BR(0x34),   1, {{OpClassCode::CALL, {R0, -1}, {I0, -1, -1, -1}, CallRelUncond< D0, S0 >}}},
    {"fbne",    MASK_BR, OPCODE_BR(0x35),   1, {{OpClassCode::fBC,  {-1, -1}, {R1, I0, -1, -1}, BranchRelCond< S1, Compare< S0, IntConst<u64, 0>, FPCondNotEqual<f64> > >}}},
    {"fbge",    MASK_BR, OPCODE_BR(0x36),   1, {{OpClassCode::fBC,  {-1, -1}, {R1, I0, -1, -1}, BranchRelCond< S1, Compare< S0, IntConst<u64, 0>, FPCondGreaterEqual<f64> > >}}},
    {"fbgt",    MASK_BR, OPCODE_BR(0x37),   1, {{OpClassCode::fBC,  {-1, -1}, {R1, I0, -1, -1}, BranchRelCond< S1, Compare< S0, IntConst<u64, 0>, FPCondGreater<f64> > >}}},

    {"blbc",    MASK_BR, OPCODE_BR(0x38),   1, {{OpClassCode::iBC,  {-1, -1}, {R1, I0, -1, -1}, BranchRelCond< S1, Compare< S0, IntConst<u64, 0>, IntCondEqualNthBit<u64,0> > >}}},
    {"beq",     MASK_BR, OPCODE_BR(0x39),   1, {{OpClassCode::iBC,  {-1, -1}, {R1, I0, -1, -1}, BranchRelCond< S1, Compare< S0, IntConst<u64, 0>, IntCondEqual<u64> > >}}},
    {"blt",     MASK_BR, OPCODE_BR(0x3a),   1, {{OpClassCode::iBC,  {-1, -1}, {R1, I0, -1, -1}, BranchRelCond< S1, Compare< S0, IntConst<u64, 0>, IntCondLessSigned<u64> > >}}},
    {"ble",     MASK_BR, OPCODE_BR(0x3b),   1, {{OpClassCode::iBC,  {-1, -1}, {R1, I0, -1, -1}, BranchRelCond< S1, Compare< S0, IntConst<u64, 0>, IntCondLessEqualSigned<u64> > >}}},
    {"blbs",    MASK_BR, OPCODE_BR(0x3c),   1, {{OpClassCode::iBC,  {-1, -1}, {R1, I0, -1, -1}, BranchRelCond< S1, Compare< S0, IntConst<u64, 0>, IntCondNotEqualNthBit<u64,0> > >}}},
    {"bne",     MASK_BR, OPCODE_BR(0x3d),   1, {{OpClassCode::iBC,  {-1, -1}, {R1, I0, -1, -1}, BranchRelCond< S1, Compare< S0, IntConst<u64, 0>, IntCondNotEqual<u64> > >}}},
    {"bge",     MASK_BR, OPCODE_BR(0x3e),   1, {{OpClassCode::iBC,  {-1, -1}, {R1, I0, -1, -1}, BranchRelCond< S1, Compare< S0, IntConst<u64, 0>, IntCondGreaterEqualSigned<u64> > >}}},
    {"bgt",     MASK_BR, OPCODE_BR(0x3f),   1, {{OpClassCode::iBC,  {-1, -1}, {R1, I0, -1, -1}, BranchRelCond< S1, Compare< S0, IntConst<u64, 0>, IntCondGreaterSigned<u64> > >}}},

};

//
// split load Store
//
Alpha64Converter::OpDef Alpha64Converter::m_OpDefsSplitLoadStore[] =
{
    {"ldq_u",   MASK_MEM, OPCODE_MEM(0x0b), 2, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddrUnaligned< S0, S1 > >},
        {OpClassCode::iLD,  {R0, -1}, {T0, -1, -1, -1}, Set< D0, Load< u64, S0 > >}
    }},
    {"stq_u",   MASK_MEM, OPCODE_MEM(0x0f), 2, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddrUnaligned< S0, S1 > >},
        {OpClassCode::iST,  {-1, -1}, {R2, T0, -1, -1}, Store< u64, S0, S1 >}
    }},

    {"ldl",     MASK_MEM, OPCODE_MEM(0x28), 2, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >},
        {OpClassCode::iLD,  {R0, -1}, {T0, -1, -1, -1}, SetSext< D0, Load< u32, S0 > >}
    }},
    {"ldq",     MASK_MEM, OPCODE_MEM(0x29), 2, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >},
        {OpClassCode::iLD,  {R0, -1}, {T0, -1, -1, -1}, Set< D0, Load< u64, S0 > >}
    }},
    {"ldl_l",       MASK_MEM, OPCODE_MEM(0x2a), 2, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >},
        {OpClassCode::iLD,  {R0, -1}, {T0, -1, -1, -1}, SetSext< D0, Load< u32, S0 > >}
    }},
    {"ldq_l",       MASK_MEM, OPCODE_MEM(0x2b), 2, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >},
        {OpClassCode::iLD,  {R0, -1}, {T0, -1, -1, -1}, Set< D0, Load< u64, S0 > >}
    }},

    {"stl",     MASK_MEM, OPCODE_MEM(0x2c), 2, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >},
        {OpClassCode::iST,  {-1, -1}, {R2, T0, -1, -1}, Store< u32, S0, S1 >}
    }},
    {"stq",     MASK_MEM, OPCODE_MEM(0x2d), 2, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >},
        {OpClassCode::iST,  {-1, -1}, {R2, T0, -1, -1}, Store< u64, S0, S1 >}
    }},
    {"stl_c",       MASK_MEM, OPCODE_MEM(0x2e), 3, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >},
        {OpClassCode::iST,  {-1, -1}, {R2, T0, -1, -1}, Store< u32, S0, S1 >},
        // R2に lock の成否を格納する必要がある．常に成功するので1
        {OpClassCode::iALU, {R2, -1}, {-1, -1, -1, -1}, Set< D0, IntConst<u64, 1> >}
    }},
    {"stq_c",       MASK_MEM, OPCODE_MEM(0x2f), 3, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >},
        {OpClassCode::iST,  {-1, -1}, {R2, T0, -1, -1}, Store< u64, S0, S1 >},
        // lock の成否を書く (同上)
        {OpClassCode::iALU, {R2, -1}, {-1, -1, -1, -1}, Set< D0, IntConst<u64, 1> >}
    }},


    // ieee float only (no ld/st-f/g)
    {"lds",     MASK_MEM, OPCODE_MEM(0x22), 2, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >},
        {OpClassCode::fLD,  {R0, -1}, {T0, -1, -1, -1}, Set< D0, AlphaLds< S0 > >}
    }},
    {"ldt",     MASK_MEM, OPCODE_MEM(0x23), 2, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >},
        {OpClassCode::fLD,  {R0, -1}, {T0, -1, -1, -1}, Set< D0, Load< u64, S0 > >}
    }},
    {"sts",     MASK_MEM, OPCODE_MEM(0x26), 2, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >},
        {OpClassCode::fST,  {-1, -1}, {R2, T0, -1, -1}, AlphaSts< S0, S1 >}
    }},
    {"stt",     MASK_MEM, OPCODE_MEM(0x27), 2, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >},
        {OpClassCode::fST,  {-1, -1}, {R2, T0, -1, -1}, Store< u64, S0, S1 >}
    }},

    // BWX
    {"ldbu",    MASK_MEM, OPCODE_MEM(0x0a), 2, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >},
        {OpClassCode::iLD,  {R0, -1}, {T0, -1, -1, -1}, Set< D0, Load< u8, S0 > >}
    }},
    {"ldwu",    MASK_MEM, OPCODE_MEM(0x0c), 2, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >},
        {OpClassCode::iLD,  {R0, -1}, {T0, -1, -1, -1}, Set< D0, Load< u16, S0 > >}
    }},
    {"stb",     MASK_MEM, OPCODE_MEM(0x0e), 2, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >},
        {OpClassCode::iST,  {-1, -1}, {R2, T0, -1, -1}, Store< u8, S0, S1 >}
    }},
    {"stw",     MASK_MEM, OPCODE_MEM(0x0d), 2, {
        {OpClassCode::ADDR, {T0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaAddr< S0, S1 > >},
        {OpClassCode::iST,  {-1, -1}, {R2, T0, -1, -1}, Store< u16, S0, S1 >}
    }},
};

//
// non-split load Store
//
Alpha64Converter::OpDef Alpha64Converter::m_OpDefsNonSplitLoadStore[] =
{
    {"ldq_u",   MASK_MEM, OPCODE_MEM(0x0b), 1, {
        {OpClassCode::iLD,      {R0, -1}, {R1, I0, -1, -1}, Set< D0, Load< u64, AlphaAddrUnaligned< S0, S1 > > >}
    }},
    {"stq_u",   MASK_MEM, OPCODE_MEM(0x0f), 1, {
        {OpClassCode::iST,      {-1, -1}, {R2, R1, I0, -1}, Store< u64, S0, AlphaAddrUnaligned< S1, S2> >}
    }},

    {"ldl",     MASK_MEM, OPCODE_MEM(0x28), 1, {
        {OpClassCode::iLD,      {R0, -1}, {R1, I0, -1, -1}, SetSext< D0, Load< u32, AlphaAddr< S0, S1 > > >}
    }},
    {"ldq",     MASK_MEM, OPCODE_MEM(0x29), 1, {
        {OpClassCode::iLD,      {R0, -1}, {R1, I0, -1, -1}, Set< D0, Load< u64, AlphaAddr< S0, S1 > > >}
    }},
    {"ldl_l",       MASK_MEM, OPCODE_MEM(0x2a), 1, {
        {OpClassCode::iLD,      {R0, -1}, {R1, I0, -1, -1}, SetSext< D0, Load< u32, AlphaAddr< S0, S1 > > >}
    }},
    {"ldq_l",       MASK_MEM, OPCODE_MEM(0x2b), 1, {
        {OpClassCode::iLD,      {R0, -1}, {R1, I0, -1, -1}, Set< D0, Load< u64, AlphaAddr< S0, S1 > > >}
    }},

    {"stl",     MASK_MEM, OPCODE_MEM(0x2c), 1, {
        {OpClassCode::iST,      {-1, -1}, {R2, R1, I0, -1}, Store< u32, S0, AlphaAddr< S1, S2> >}
    }},
    {"stq",     MASK_MEM, OPCODE_MEM(0x2d), 1, {
        {OpClassCode::iST,      {-1, -1}, {R2, R1, I0, -1}, Store< u64, S0, AlphaAddr< S1, S2> >}
    }},
    {"stl_c",       MASK_MEM, OPCODE_MEM(0x2e), 2, {
        {OpClassCode::iST,      {-1, -1}, {R2, R1, I0, -1}, Store< u32, S0, AlphaAddr< S1, S2> >},
        // R2に lock の成否を格納する必要がある．常に成功するので1
        {OpClassCode::iALU, {R2, -1}, {-1, -1, -1, -1}, Set< D0, IntConst<u64, 1> >}
    }},
    {"stq_c",       MASK_MEM, OPCODE_MEM(0x2f), 2, {
        {OpClassCode::iST,      {-1, -1}, {R2, R1, I0, -1}, Store< u64, S0, AlphaAddr< S1, S2> >},
        // lock の成否を書く (同上)
        {OpClassCode::iALU, {R2, -1}, {-1, -1, -1, -1}, Set< D0, IntConst<u64, 1> >}
    }},

    // ieee float only (no ld/st-f/g)
    {"lds",     MASK_MEM, OPCODE_MEM(0x22), 1, {
        {OpClassCode::fLD,      {R0, -1}, {R1, I0, -1, -1}, Set< D0, AlphaLds< AlphaAddr< S0, S1 > > >}
    }},
    {"ldt",     MASK_MEM, OPCODE_MEM(0x23), 1, {
        {OpClassCode::fLD,      {R0, -1}, {R1, I0, -1, -1}, Set< D0, Load< u64, AlphaAddr< S0, S1 > > >}
    }},
    {"sts",     MASK_MEM, OPCODE_MEM(0x26), 1, {
        {OpClassCode::fST,      {-1, -1}, {R2, R1, I0, -1}, AlphaSts< S0, AlphaAddr< S1, S2 > >}
    }},
    {"stt",     MASK_MEM, OPCODE_MEM(0x27), 1, {
        {OpClassCode::fST,      {-1, -1}, {R2, R1, I0, -1}, Store< u64, S0, AlphaAddr< S1, S2 > >}
    }},


    // BWX
    {"ldbu",        MASK_MEM, OPCODE_MEM(0x0a), 1, {
        {OpClassCode::iLD,      {R0, -1}, {R1, I0, -1, -1}, Set< D0, Load< u8, AlphaAddr< S0, S1 > > >}
    }},
    {"ldwu",        MASK_MEM, OPCODE_MEM(0x0c), 1, {
        {OpClassCode::iLD,      {R0, -1}, {R1, I0, -1, -1}, Set< D0, Load< u16, AlphaAddr< S0, S1 > > >}
    }},
    {"stb",     MASK_MEM, OPCODE_MEM(0x0e), 1, {
        {OpClassCode::iST,      {-1, -1}, {R2, R1, I0, -1}, Store< u8, S0, AlphaAddr< S1, S2> >}
    }},
    {"stw",     MASK_MEM, OPCODE_MEM(0x0d), 1, {
        {OpClassCode::iST,      {-1, -1}, {R2, R1, I0, -1}, Store< u16, S0, AlphaAddr< S1, S2> >}
    }},
};


//
// Alpha64Converter
//


Alpha64Converter::Alpha64Converter()
{
    AddToOpMap(m_OpDefsBase, sizeof(m_OpDefsBase)/sizeof(OpDef));
    if (IsSplitLoadStoreEnabled()) {
        AddToOpMap(m_OpDefsSplitLoadStore, sizeof(m_OpDefsSplitLoadStore)/sizeof(OpDef));
    }
    else{
        AddToOpMap(m_OpDefsNonSplitLoadStore, sizeof(m_OpDefsNonSplitLoadStore)/sizeof(OpDef));
    }
}

Alpha64Converter::~Alpha64Converter()
{
}

// srcTemplate に対応するオペランドの種類と，レジスタならば番号を，即値ならばindexを返す
std::pair<Alpha64Converter::OperandType, int> Alpha64Converter::GetActualSrcOperand(int srcTemplate, const DecodedInsn& decoded) const
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
int Alpha64Converter::GetActualRegNumber(int regTemplate, const DecodedInsn& decoded) const
{
    if (regTemplate == -1) {
        return -1;
    }
    else if (RegTemplateBegin <= regTemplate && regTemplate <= RegTemplateEnd) {
        return decoded.Reg[regTemplate - RegTemplateBegin];
    }
    else if (0 <= regTemplate && regTemplate < Alpha64Info::RegisterCount) {
        return regTemplate;
    }
    else {
        ASSERT(0, "Alpha64Converter Logic Error : invalid regTemplate");
        return -1;
    }
}

// レジスタ番号regNumがゼロレジスタかどうかを判定する
bool Alpha64Converter::IsZeroReg(int regNum) const
{
    const int IntZeroReg = 31;
    const int FPZeroReg = 63;

    return regNum == IntZeroReg || regNum == FPZeroReg;
}


void Alpha64Converter::AlphaUnknownOperation(OpEmulationState* opState)
{
    u32 codeWord = static_cast<u32>( SrcOperand<0>()(opState) );

    DecoderType decoder;
    DecodedInsn decoded;
    decoder.Decode( codeWord, &decoded);

    stringstream ss;
    u32 opcode = (codeWord >> 26) & 0x3f;
    ss << "unknown instruction : " << hex << setw(8) << codeWord << endl;
    ss << "\topcode : " << hex << opcode << endl;
    ss << "\timm[1] : " << hex << decoded.Imm[1] << endl;

    THROW_RUNTIME_ERROR(ss.str().c_str());
}

const Alpha64Converter::OpDef* Alpha64Converter::GetOpDefUnknown() const
{
    return &m_OpDefUnknown;
}
