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

#include "Emu/PPC64Linux/PPC64Decoder.h"

#include "SysDeps/Endian.h"
#include "Emu/Utility/DecoderUtility.h"

// References:
//   PowerISA, http://www.power.org/ (search PowerISA)
//   PDF中，ビットのIndexがMSB=0で表記されているのに注意

using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::PPC64Linux;

// 本来はPPC64Decoderのprivateメンバにすべきだが，
// 面倒だったので手抜きして無名ネームスペースにしてしまった
namespace { 
    // 命令語中の各オペランドをDecodeInfoElemの配列として定義し，
    // DecodeInfoElem に従ってオペランドを取り出す


    // CR bitの指定は，CR番号とCRn内でのbitに分解

    // オペランド定義で，DecodedInsnへの書き込み先
    const int NONE = 0;
    // Reg
    const int D_R0 =  8;
    const int D_R1 =  9;
    const int D_R2 = 10;
    const int D_R3 = 11;
    // Imm
    const int D_I0 = 16;
    const int D_I1 = 17;
    const int D_I2 = 18;
    const int D_I3 = 19;
    
    // オペランドの種類
    enum OperandType {
        // Reg
        GPR,    // GPR number
        FPR,    // FPR number
        CRN,    // CR number
//      SPR,    // SPR number
        // special form
        // (no special form)

        // Imm
        CRI,    // CR bit index
        UIM,    // unsigned imm
        SIM,    // signed imm

        // special form
        SH6,    // MD-Form/XS-Form 6-bit shift count
        MB5,    // MD-Form/MDS-Form 5-bit mask begin/end
    };

    struct OperandFormat {
        OperandType Type;   // オペランドの種類
        int BitIndex;       // CodeWord 中の位置 (LSB=0)
        int BitCount;       // ビット数
    };
    struct DecodeInfoElem {
        int DecodedPlace;   // DecodedInsn 中の位置
        OperandFormat Operand;  // 
    };

    static const int MaxOperands = 6;
    struct DecodeInfo {
        struct DecodeInfoElem DecodeInfoElem[MaxOperands];
    };

    // よくあるオペランドを定義
    const OperandFormat GR0 = {GPR, 21, 5}; // GPR  6:10
    const OperandFormat GR1 = {GPR, 16, 5}; // GPR 11:15
    const OperandFormat GR2 = {GPR, 11, 5}; // GPR 16:20

    const OperandFormat FR0 = {FPR, 21, 5}; // FPR  6:10
    const OperandFormat FR1 = {FPR, 16, 5}; // FPR 11:15
    const OperandFormat FR2 = {FPR, 11, 5}; // FPR 16:20
    const OperandFormat FR3 = {FPR,  6, 5}; // FPR 21:25

    // その他Index
    const OperandFormat IX0 = {UIM, 21, 5}; // UIM  6:10
    const OperandFormat IX1 = {UIM, 16, 5}; // UIM 11:15
    const OperandFormat IX2 = {UIM, 11, 5}; // UIM 16:20
    const OperandFormat IX3 = {UIM,  6, 5}; // UIM 21:25
    const OperandFormat IX4 = {UIM,  1, 5}; // UIM 26:30

    const OperandFormat UI16 = {UIM,  0, 16};   // UIM 16:31
    const OperandFormat SI16 = {SIM,  0, 16};   // SIM 16:31
    const OperandFormat IDS = {SIM,  2, 14};    // SIM 16:29 : DS-Formのdisplacement

    const OperandFormat IMB5 = {MB5,  0, 0};    // UIM 26:26 || UIM21:25
    const OperandFormat ISH6 = {SH6,  0, 0};    // UIM 30:30 || UIM16:20

    const OperandFormat IFLM = {UIM, 17, 8};    // UIM  7:14

    // CR Number
    const OperandFormat CR0 = {CRN, 23, 3}; // CRN  6: 8
    const OperandFormat CR1 = {CRN, 18, 3}; // CRN 11:13
    const OperandFormat CR2 = {CRN, 13, 3}; // CRN 16:18
    const OperandFormat CR3 = {CRN,  8, 3}; // CRN 21:23

    // CR Bit
    const OperandFormat CI0 = {UIM, 21, 2}; // UIM  9:10
    const OperandFormat CI1 = {UIM, 16, 2}; // UIM 14:15
    const OperandFormat CI2 = {UIM, 11, 2}; // UIM 19:20
    const OperandFormat CI3 = {UIM,  6, 2}; // UIM 24:25

    //
    // Form
    //
    const DecodeInfo UND = {};
    const DecodeInfo EXT = {};  // 拡張オペコード

    // I-Form
    const DecodeInfo IFRM = {{{D_I0, {SIM, 2, 24}}}};

    // B-Form
    const DecodeInfo BFRM = {{{D_R0, CR1}, {D_I0, CI1}, {D_I1, {SIM, 2, 14}}}};

    // A-Form : GR, GR, GR, CR (isel)
    const DecodeInfo AGGGC = {{{D_R0, GR0}, {D_R1, GR1}, {D_R2, GR2}, {D_R3, CR3}, {D_I0, CI3}}};
    // A-Form : FR, FR, FR, FR
    const DecodeInfo AFFFF = {{{D_R0, FR0}, {D_R1, FR1}, {D_R2, FR2}, {D_R3, FR3}}};
    // A-Form : FR, FR, FR, __
    const DecodeInfo AFFF_ = {{{D_R0, FR0}, {D_R1, FR1}, {D_R2, FR2}}};
    // A-Form : FR, FR, __, FR
    const DecodeInfo AFF_F = {{{D_R0, FR0}, {D_R1, FR1}, {D_R2, FR3}}};
    // A-Form : FR, __, FR, FR
    const DecodeInfo AF_F_ = {{{D_R0, FR0}, {D_R1, FR2}}};

    // SC-Form (takes no operand)
    const DecodeInfo SCFRM = {};

    // D-Form
    const DecodeInfo IDRU = {{{D_R0, GR0}, {D_R1, GR1}, {D_I0, UI16}}};
    const DecodeInfo IDRS = {{{D_R0, GR0}, {D_R1, GR1}, {D_I0, SI16}}};
    const DecodeInfo IDRD = IDRS;
    // D-Form : Compare
    const DecodeInfo IDCU = {{{D_R0, CR0}, {D_R1, GR1}, {D_I0, UI16}}};
    const DecodeInfo IDCS = {{{D_R0, CR0}, {D_R1, GR1}, {D_I0, SI16}}};
    const DecodeInfo IDCD = IDCS;
    // D-Form : IX0, R1, SI16 ( tdi, twi)
    const DecodeInfo IDXS = {{{D_R1, GR1}, {D_I0, IX0}, {D_I1, SI16}}};
    // D-Form : FP Load/Store
    const DecodeInfo FDRS = {{{D_R0, FR0}, {D_R1, GR1}, {D_I0, SI16}}};
    const DecodeInfo FDRD = FDRS;

    // DS-Form : Int Load/Store
    const DecodeInfo DSFRM = {{{D_R0, GR0}, {D_R1, GR1}, {D_I0, IDS}}};

    // XL-Form : conditional branch to LR/CTR (bclr, bcctr)
    const DecodeInfo XLBR = {{{D_R0, CR1}, {D_I0, CI1}}};
    // XL-Form : CR bit-wise operations
    const DecodeInfo XLCRB = {{{D_R0, CR0}, {D_I0, CI0}, {D_R1, CR1}, {D_I1, CI1}, {D_R2, CR2}, {D_I2, CI2}}};
    // XL-Form : CR move operations
    const DecodeInfo XLCR = {{{D_R0, CR0}, {D_R1, CR1}}};
    // XL-Form no operand
    const DecodeInfo XL__ = {};

    // X-Form : GR, __, __ ; mtspr, mfspr
    const DecodeInfo XG__ = {{{D_R0, GR0}}};
    // X-Form : GR, FXM ; mt(o)crf, mf(o)crf, mtcr, mfcr
    const DecodeInfo XGFXM = {{{D_R0, GR0}, {D_I1, {UIM, 12, 8}}}};
    // X-Form : GR, GR, __
    const DecodeInfo XGG_ = {{{D_R0, GR0}, {D_R1, GR1}}};
    // X-Form : GR, GR, GR
    const DecodeInfo XGGG = {{{D_R0, GR0}, {D_R1, GR1}, {D_R2, GR2}}};
    // X-Form : GR, GR, IX
    const DecodeInfo XGGI = {{{D_R0, GR0}, {D_R1, GR1}, {D_I0, IX2}}};
    // X-Form : GR, GR, SH
    const DecodeInfo XGGS = {{{D_R0, GR0}, {D_R1, GR1}, {D_I0, ISH6}}};
    // X-Form : CR, GR, GR ;compare
    const DecodeInfo XCMP = {{{D_R0, CR0}, {D_R1, GR1}, {D_R2, GR2}}};
    // X-Form : IX, GR, GR
    const DecodeInfo XIGG = {{{D_I0, IX0}, {D_R0, GR1}, {D_R1, GR2}}};
    // X-Form : __, GR, GR
    const DecodeInfo X_GG = {{{D_R0, GR1}, {D_R1, GR2}}};
    // X-Form : __, __, __
    const DecodeInfo X___ = {};
    // X-Form : CN, __, __
    const DecodeInfo XN__ = {{{D_R0, CR0}}};
    // X-Form : FR, GR, GR; FP load/store
    const DecodeInfo XFGG = {{{D_R0, FR0}, {D_R1, GR1}, {D_R2, GR2}}};
    // X-Form : FR, FR, FR
    const DecodeInfo XFFF = {{{D_R0, FR0}, {D_R1, FR1}, {D_R2, FR2}}};
    // X-Form : FR, __, FR
    const DecodeInfo XF_F = {{{D_R0, FR0}, {D_R1, FR2}}};
    // X-Form : FR, __, __
    const DecodeInfo XF__ = {{{D_R0, FR0}}};
    // X-Form : CR, FR, FR ; FP compare
    const DecodeInfo XFCMP = {{{D_R0, CR0}, {D_R1, FR1}, {D_R2, FR2}}};
    // X-Form : IX, __, __
    const DecodeInfo XI__ = {{{D_I0, IX0}}};

    // M-Form (shift count from register)
    const DecodeInfo MREG = {{{D_R0, GR0}, {D_R1, GR1}, {D_R2, GR2}, {D_I0, IX3}, {D_I1, IX4}}};
    // M-Form (shift count from immediate)
    const DecodeInfo MIMM = {{{D_R0, GR0}, {D_R1, GR1}, {D_I0, IX2}, {D_I1, IX3}, {D_I2, IX4}}};

    // MD-Form
    const DecodeInfo MDFRM = {{{D_R0, GR0}, {D_R1, GR1}, {D_I0, ISH6}, {D_I1, IMB5}}};
    // MDS-Form
    const DecodeInfo MDSFRM = {{{D_R0, GR0}, {D_R1, GR1}, {D_R2, GR2}, {D_I0, IMB5}}};

    // 命令コードとオペランド定義
    const DecodeInfo OpCodeToDecodeInfo[64] =
    {
        // 0x00
        UND   , UND   , IDXS  , IDXS  , UND   , UND   , UND   , IDRS  ,
        // 0x08
        IDRS  , UND   , IDCU  , IDCS  , IDRS  , IDRS  , IDRS  , IDRS  ,
        // 0x10
        BFRM  , SCFRM , IFRM  , EXT   , MIMM  , MIMM  , UND   , MREG  ,
        // 0x18
        IDRU  , IDRU  , IDRU  , IDRU  , IDRU  , IDRU  , EXT   , EXT   ,
        // 0x20
        IDRD  , IDRD  , IDRD  , IDRD  , IDRD  , IDRD  , IDRD  , IDRD  ,
        // 0x28
        IDRD  , IDRD  , IDRD  , IDRD  , IDRD  , IDRD  , IDRD  , IDRD  ,
        // 0x30
        FDRD  , FDRD  , FDRD  , FDRD  , FDRD  , FDRD  , FDRD  , FDRD  ,
        // 0x38
        UND   , UND   , EXT   , EXT   , UND   , UND   , EXT   , EXT   ,
    };
    // lq (56), stq(62.2) はサポートしない (ついでにこれらはprivileged)

    // Opcode と対応するDecodeInfoの組 (配列の初期化を行うため)
    struct OpcodeDecodeInfoPair {
        u32 Opcode;
        struct DecodeInfo DecodeInfo;
    };

    const DecodeInfo OpCode30ExtToDecodeInfo[16] = {
        // 0x00
        MDFRM , MDFRM , MDFRM , MDFRM , MDFRM , MDFRM , MDFRM , MDFRM , 
        // 0x08
        MDSFRM, MDSFRM, UND   , UND   , UND   , UND   , UND   , UND   ,
    };
    const DecodeInfo OpCode58ExtToDecodeInfo[4] = {
        DSFRM , DSFRM , DSFRM , UND , 
    };
    const DecodeInfo OpCode62ExtToDecodeInfo[4] = {
        DSFRM , DSFRM , UND   , UND , 
    };

    const OpcodeDecodeInfoPair OpCode19ExtToDecodeInfo[] = {
        {   0, XLCR  }, // mcrf
        {  16, XLBR  }, // bclr
        {  33, XLCRB }, // crnor
        { 129, XLCRB }, // crandc
        { 150, XL__  }, // isync
        { 193, XLCRB }, // crxor
        { 225, XLCRB }, // crnand
        { 257, XLCRB }, // crand
        { 289, XLCRB }, // creqv
        { 417, XLCRB }, // crorc
        { 449, XLCRB }, // cror
        { 528, XLBR  }, // bcctr
    };
    const OpcodeDecodeInfoPair OpCode31ExtToDecodeInfo[] = {
        {   0, XCMP  }, // cmp (d/w)
        {   4, XIGG  }, // tw
        {   8, XGGG  }, // subfc
        {   9, XGGG  }, // mulhdu
        {  10, XGGG  }, // addc
        {  11, XGGG  }, // mulhwu
        {  19, XGFXM }, // mfcr
        {  20, XGGG  }, // lwarx
        {  21, XGGG  }, // ldx
        {  23, XGGG  }, // lwzx
        {  24, XGGG  }, // slw
        {  26, XGG_  }, // cntlzw
        {  27, XGGG  }, // sld
        {  28, XGGG  }, // and

        {  32, XCMP  }, // cmpl (d/w)
        {  40, XGGG  }, // subf
        {  53, XGGG  }, // ldux
        {  54, X_GG  }, // dcbst
        {  55, XGGG  }, // lwzux
        {  58, XGG_  }, // cntlzd
        {  60, XGGG  }, // andc

        {  68, XIGG  }, // td
        {  73, XGGG  }, // mulhd
        {  75, XGGG  }, // mulhw
        {  83, XG__  }, // mfmsr
        {  84, XGGG  }, // ldarx
        {  86, X_GG  }, // dcbf
        {  87, XGGG  }, // lbzx

        { 104, XGG_  }, // neg
        { 119, XGGG  }, // lbzux
        { 122, XGG_  }, // popcntb
        { 124, XGGG  }, // nor

        { 136, XGGG  }, // subfe
        { 138, XGGG  }, // adde
        { 144, XGFXM }, // mtcrf
        { 149, XGGG  }, // stdx
        { 150, XGGG  }, // stwcx.
        { 151, XGGG  }, // stwx

        { 181, XGGG  }, // stdux
        { 183, XGGG  }, // stwux

        { 200, XGG_  }, // subfze
        { 202, XGG_  }, // addze
        { 214, XGGG  }, // stdcx.
        { 215, XGGG  }, // stbx

        { 232, XGG_  }, // subfme
        { 233, XGGG  }, // mulld
        { 234, XGG_  }, // addme
        { 235, XGGG  }, // mullw
        { 246, X_GG  }, // dcbtst
        { 247, XGGG  }, // stbux

        { 266, XGGG  }, // add
        { 278, X_GG  }, // dcbt
        { 279, XGGG  }, // lhzx
        { 284, XGGG  }, // eqv

        { 311, XGGG  }, // lhzux
        { 316, XGGG  }, // xor

        { 339, XG__  }, // mfspr
        { 341, XGGG  }, // lwax
        { 343, XGGG  }, // lhax

        { 371, XG__  }, // mftb
        { 373, XGGG  }, // lwaux
        { 375, XGGG  }, // lhaux

        { 407, XGGG  }, // sthx
        { 412, XGGG  }, // orc

        { 439, XGGG  }, // sthux
        { 444, XGGG  }, // or

        { 457, XGGG  }, // divdu
        { 459, XGGG  }, // divwu
        { 467, XG__  }, // mtspr
        { 476, XGGG  }, // nand

        { 489, XGGG  }, // divd
        { 491, XGGG  }, // divw

        { 512, XN__  }, // mcrxr
        { 520, XGGG  }, // subfco
        { 521, XGGG  }, // mulhdu
        { 522, XGGG  }, // addco
        { 523, XGGG  }, // mulhwu
        { 533, XGGG  }, // lswx
        { 534, XGGG  }, // lwbrx
        { 535, XFGG  }, // lfsx
        { 536, XGGG  }, // srw
        { 539, XGGG  }, // srd

        { 552, XGGG  }, // subfo
        { 567, XFGG  }, // lfsux

        { 585, XGGG  }, // mulhd
        { 587, XGGG  }, // mulhw
        { 597, XGGI  }, // lswi
        { 598, X___  }, // sync
        { 599, XFGG  }, // lfdx

        { 616, XGG_  }, // nego
        { 631, XFGG  }, // lfdux

        { 648, XGGG  }, // subfeo
        { 650, XGGG  }, // addeo
        { 661, XGGG  }, // stswx
        { 662, XGGG  }, // stwbrx
        { 663, XFGG  }, // stfsx

        { 695, XFGG  }, // stfsux

        { 712, XGG_  }, // subfzeo
        { 714, XGG_  }, // addzeo
        { 725, XGGI  }, // stswi
        { 727, XFGG  }, // stfdx

        { 744, XGG_  }, // subfmeo
        { 745, XGGG  }, // mulldo
        { 746, XGG_  }, // addmeo
        { 747, XGGG  }, // mullwo
        { 759, XFGG  }, // stdux

        { 778, XGGG  }, // addo
        { 790, XGGG  }, // lhbrx
        { 792, XGGG  }, // sraw
        { 794, XGGG  }, // srad

        { 824, XGGI  }, // srawi
        { 826, XGGS  }, // sradi
        { 827, XGGS  }, // sradi

        { 854, X___  }, // eieio

        { 918, XGGG  }, // sthbrx
        { 922, XGG_  }, // extsh

        { 954, XGG_  }, // extsb

        { 969, XGGG  }, // divduo
        { 971, XGGG  }, // divwuo
        { 982, X_GG  }, // icbi
        { 983, XFGG  }, // stfiwx
        { 986, XGG_  }, // extsw

        {1001, XGGG  }, // divdo
        {1003, XGGG  }, // divwo
        {1014, X_GG  }, // dcbz
//      {   0, X     }, //
    };
    const OpcodeDecodeInfoPair OpCode59ExtToDecodeInfo[] = {
        {0, UND},   // 命令なし
    };
    const OpcodeDecodeInfoPair OpCode63ExtToDecodeInfo[] = {
        {   0, XFCMP }, // fcmpu
        {  12, XF_F  }, // frsp
        {  14, XF_F  }, // fctiw
        {  15, XF_F  }, // fctiwz

        {  32, XFCMP }, // fcmpo
        {  38, XI__  }, // mtfsb1
        {  40, XF_F  }, // fneg

        {  64, {{{D_R0, CR0},{D_I0, {UIM, 18, 3}}}} },  // mcrfs
        {  70, XI__  }, // mtfsb0
        {  72, XF_F  }, // fmr

        { 134, {{{D_I0, {UIM, 23, 3}}, {D_I1, {UIM, 12, 4}}}} },    // mtfsfi
        { 136, XF_F  }, // fnabs

        { 264, XF_F  }, // fabs

        { 392, XF_F  }, // frin
        { 424, XF_F  }, // friz
        { 456, XF_F  }, // frip
        { 488, XF_F  }, // frim

        { 583, XF__  }, // mffs
        { 711, {{{D_I0, IFLM}, {D_R0, FR2}}}},  // mtfsf

        { 814, XF_F  }, // fctid
        { 815, XF_F  }, // fctidz
        { 846, XF_F  }, // fcfid
    };

    // Opcode59 のうち，bit 21:25 をFRCに使うかdon't careなもの (bit26が1のもの)
    const DecodeInfo OpCode59FRCExtToDecodeInfo[16] = {
        // 0x00
        UND   , UND   , AFFF_ , UND   , AFFF_ , AFFF_ , AF_F_ , UND   ,
        // 0x08
        AF_F_ , AFF_F , AF_F_ , UND   , AFFFF , AFFFF , AFFFF , AFFFF ,
    };
    // Opcode63 のうち，bit 21:25 をFRCに使うかdon't careなもの (bit26が1のもの)
    const DecodeInfo OpCode63FRCExtToDecodeInfo[16] = {
        // 0x00
        UND   , UND   , AFFF_ , UND   , AFFF_ , AFFF_ , AF_F_ , AFFFF  ,
        // 0x08
        AF_F_ , AFF_F , AF_F_ , UND   , AFFFF , AFFFF , AFFFF , AFFFF ,
    };

    // OpCodeXXExtToDecodeInfo のbeginとendを定義
#define MakeBeginAndEnd(n) \
    const OpcodeDecodeInfoPair* const OpCode##n##ExtToDecodeInfoBegin = OpCode##n##ExtToDecodeInfo; \
    const OpcodeDecodeInfoPair* const OpCode##n##ExtToDecodeInfoEnd = OpCode##n##ExtToDecodeInfoBegin + sizeof(OpCode##n##ExtToDecodeInfo)/sizeof(OpCode##n##ExtToDecodeInfo[0]);

    MakeBeginAndEnd(19);
    MakeBeginAndEnd(31);
    MakeBeginAndEnd(59);
    MakeBeginAndEnd(63);
#undef MakeBeginAndEnd

    struct OpcodeDecodeInfoPairLess : public binary_function< OpcodeDecodeInfoPair, OpcodeDecodeInfoPair, bool >
    {
        bool operator()(const OpcodeDecodeInfoPair& lhs, const OpcodeDecodeInfoPair& rhs)
        {
            return lhs.Opcode < rhs.Opcode;
        }
    };

    const DecodeInfo& FindDecodeInfo(const OpcodeDecodeInfoPair* first, const OpcodeDecodeInfoPair* last, u32 xo)
    {
        OpcodeDecodeInfoPair val;
        val.Opcode = xo;

        const OpcodeDecodeInfoPair* e = lower_bound(first, last, val, OpcodeDecodeInfoPairLess());

        if (e == last)
            return UND;
        else
            return e->DecodeInfo;
    }

    const DecodeInfo& GetDecodeInfo(u32 codeWord)
    {
        u32 opcode = (codeWord >> 26) & 0x3f;

        // 拡張オペコードを持つものは特別処理
        switch (opcode) {
        case 30:
            {
                u32 xo = (codeWord >> 1) & 0x0f;
                return OpCode30ExtToDecodeInfo[xo];
            }
        case 58:
            {
                u32 xo = codeWord & 0x03;
                return OpCode58ExtToDecodeInfo[xo];
            }
        case 62:
            {
                u32 xo = codeWord & 0x03;
                return OpCode62ExtToDecodeInfo[xo];
            }
        case 19:
            {
                u32 xo = (codeWord >> 1) & 0x3ff;
                return FindDecodeInfo(OpCode19ExtToDecodeInfoBegin, OpCode19ExtToDecodeInfoEnd, xo);
            }
        case 31:
            {
                u32 xo = (codeWord >> 1) & 0x3ff;
                if ((xo & 0x00f) == 0x00f)
                    return AGGGC;   // isel
                else
                    return FindDecodeInfo(OpCode31ExtToDecodeInfoBegin, OpCode31ExtToDecodeInfoEnd, xo);
            }
        case 59:
            {
                u32 xo = (codeWord >> 1) & 0x3ff;
                if (xo & 0x010)
                    return OpCode59FRCExtToDecodeInfo[xo & 0x00f];
                else
                    return FindDecodeInfo(OpCode59ExtToDecodeInfoBegin, OpCode59ExtToDecodeInfoEnd, xo);
            }
        case 63:
            {
                u32 xo = (codeWord >> 1) & 0x3ff;
                if (xo & 0x010)
                    return OpCode63FRCExtToDecodeInfo[xo & 0x00f];
                else
                    return FindDecodeInfo(OpCode63ExtToDecodeInfoBegin, OpCode63ExtToDecodeInfoEnd, xo);
            }
        default:
            return OpCodeToDecodeInfo[opcode];
        }
    }
}

PPC64Decoder::DecodedInsn::DecodedInsn()
{
    clear();
}

void PPC64Decoder::DecodedInsn::clear()
{
    CodeWord = 0;

    std::fill(Imm.begin(), Imm.end(), 0);
    std::fill(Reg.begin(), Reg.end(), -1);
}


PPC64Decoder::PPC64Decoder()
{
}

void PPC64Decoder::Decode(u32 codeWord, DecodedInsn* out)
{
    out->clear();

    out->CodeWord = codeWord;

    const DecodeInfo& decodeInfo = GetDecodeInfo(codeWord);

    for (int i = 0; i < MaxOperands; i ++) {
        const OperandFormat& operand = decodeInfo.DecodeInfoElem[i].Operand;
        const int decodedPlace = decodeInfo.DecodeInfoElem[i].DecodedPlace;

        if (decodedPlace == NONE) {
            break;
        }

        u64 value = 0;

        // OperandFormat に従ってcodeWord中の一部分を取り出す
        switch (operand.Type) {
        case GPR:
            value = ExtractBits<u64>(codeWord, operand.BitIndex, operand.BitCount) + GP_REG0;
            break;
        case FPR:
            value = ExtractBits<u64>(codeWord, operand.BitIndex, operand.BitCount) + FP_REG0;
            break;
        case CRN:
            value = ExtractBits<u64>(codeWord, operand.BitIndex, operand.BitCount) + COND_REG0;
            break;
        //case SPR:
        //  break;
        case CRI:
        case UIM:
            value = ExtractBits<u64>(codeWord, operand.BitIndex, operand.BitCount);
            break;
        case SIM:
            value = ExtractBits<u64>(codeWord, operand.BitIndex, operand.BitCount, true);
            break;
        case SH6:
            value = (ExtractBits<u64>(codeWord, 1, 1) << 5) | ExtractBits(codeWord, 11, 5);
            break;
        case MB5:
            value = (ExtractBits<u64>(codeWord, 5, 1) << 5) | ExtractBits(codeWord, 6, 5);
            break;
        }

        if (D_R0 <= decodedPlace && decodedPlace <= D_R3) {
            // Reg
            out->Reg[decodedPlace-D_R0] = static_cast<int>(value);
        }
        else if (D_I0 <= decodedPlace && decodedPlace <= D_I3) {
            // Imm
            out->Imm[decodedPlace-D_I0] = value;
        }
    }
}

