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


#ifndef PPC64_LINUX_PPC64LINUX_PPC64OPERATION_H
#define PPC64_LINUX_PPC64LINUX_PPC64OPERATION_H

#include "SysDeps/fenv.h"
#include "Lib/shttl/bit.h"
#include "Emu/Utility/GenericOperation.h"
#include "Emu/Utility/System/Syscall/SyscallConvIF.h"
#include "Emu/Utility/System/ProcessState.h"

namespace Onikiri {
namespace PPC64Linux {
namespace Operation {

using namespace EmulatorUtility;
using namespace EmulatorUtility::Operation;

// **********************************
//              Utility
// **********************************

// フラグを計算する．XER[SO] は使用されないので無視
template <typename Type>
inline int PPC64CalcFlag(Type lhs, Type rhs = 0)
{
    if (lhs == rhs) {
        return 0x2;
    }
    else if (lhs > rhs) {
        return 0x4;
    }
    else if (lhs < rhs) {
        return 0x8;
    }

    return 0;   // never reached
}

// FP compare 時のフラグを計算する．
template <typename Type>
inline int PPC64CalcFlagFP(Type lhs, Type rhs = 0)
{
    if (IsNAN(lhs) || IsNAN(rhs)) {
        return 0x1;
    }
    else if (lhs == rhs) {
        return 0x2;
    }
    else if (lhs > rhs) {
        return 0x4;
    }
    else if (lhs < rhs) {
        return 0x8;
    }

    return 0;   // never reached
}

// rlwinm 等のためのマスクを生成する (MSB=0)
template <typename Type>
inline Type PPC64GenMask(unsigned int mb, unsigned int me)
{
    unsigned int typeBits = sizeof(Type)*8;

    if (mb > me) {
        // ビットmeからmbまで（いずれも含まず）が0であるビット列を得る
        return ~(Type)(shttl::mask(0, mb-me-1) << (typeBits-mb));
    }
    else {
        // ビットmbからmeまで（いずれも含む）が1であるビット列を得る
        return (Type)(shttl::mask(0, me-mb+1) << (typeBits-me-1));
    }
}

// FPSCRのFEX, VX を設定する
inline u64 PPC64AdjustFPSCR(u64 fpscr)
{
    fpscr &= 0x9fffffff;    // FEX, VXをクリア

    if (fpscr & 0x00000f80)
        fpscr |= 0x40000000;    // FEX セット
    if (fpscr & 0x01807000)
        fpscr |= 0x20000000;    // VX セット

    return fpscr;
}

// FPSCR から 丸めモードを取得する
template <typename TFPSCR>
struct PPC64FPSCRRoundMode : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    int operator()(EmulatorUtility::OpEmulationState* opState)
    {
        switch (TFPSCR()(opState) & 0x3) {
        case 0:
            return FE_TONEAREST;
        case 1:
            return FE_TOWARDZERO;
        case 2:
            return FE_UPWARD;
        case 3:
            return FE_DOWNWARD;
        default:
            ASSERT(0);  // never reached
            return FE_ROUNDDEFAULT;
        }
    }
};



// CR (TSrcCR) の TSrcBit 目 (MSB=0) を取得する
template <typename TSrcCR, typename TSrcBit>
struct PPC64CRBit : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        ASSERT(TSrcBit()(opState) < 4);
        return (TSrcCR()(opState) >> (3-TSrcBit()(opState))) & 0x1;
    }
};

// CTR (TSrcCTR) をデクリメントし，TDestCTRに代入し，デクリメントした結果を返す
template <typename TDestCTR, typename TSrcCTR>
struct PPC64DecCTR : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        u64 result = TSrcCTR()(opState) - 1;
        TDestCTR::SetOperand(opState, result);
        return result;
    }
};

// **********************************
//            system call
// **********************************
inline void PPC64SyscallSetArg(EmulatorUtility::OpEmulationState* opState)
{
    EmulatorUtility::SyscallConvIF* syscallConv = opState->GetProcessState()->GetSyscallConv();
    syscallConv->SetArg(0, SrcOperand<0>()(opState));
    syscallConv->SetArg(1, SrcOperand<1>()(opState));
    syscallConv->SetArg(2, SrcOperand<2>()(opState));
}

// invoke syscall, get result&error and branch if any
inline void PPC64SyscallCore(EmulatorUtility::OpEmulationState* opState)
{
    EmulatorUtility::SyscallConvIF* syscallConv = opState->GetProcessState()->GetSyscallConv();
    syscallConv->SetArg(3, SrcOperand<0>()(opState));
    syscallConv->SetArg(4, SrcOperand<1>()(opState));
    syscallConv->SetArg(5, SrcOperand<2>()(opState));
    syscallConv->SetArg(6, SrcOperand<3>()(opState));
    syscallConv->Execute(opState);

    DstOperand<0>::SetOperand(opState, syscallConv->GetResult(EmulatorUtility::SyscallConvIF::RetValueIndex) );
    if (syscallConv->GetResult(EmulatorUtility::SyscallConvIF::ErrorFlagIndex))
        DstOperand<1>::SetOperand(opState, 1);  // set SO
    else
        DstOperand<1>::SetOperand(opState, 0);
}

// **********************************
//              others
// **********************************

// cntlzw, cntlzd
template <typename Type, typename TSrc>
struct PPC64Cntlz : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        int lz = sizeof(Type)*8;    // leading zeros
        Type value = static_cast<Type>( TSrc()(opState) );  

        // value を n ビット右シフトして初めて0になったとき，leading zeros は sizeof(Type)*8 - n
        while (value != 0) {
            lz --;
            value >>= 1;
        }

        return lz;
    }
};

// rlwinm等
template <typename Type, typename TSrc, typename TMaskBegin, typename TMaskEnd>
struct PPC64Mask : public std::unary_function<EmulatorUtility::OpEmulationState*, Type>
{
    Type operator()(EmulatorUtility::OpEmulationState* opState)
    {
        unsigned int typeBits = sizeof(Type)*8;
        unsigned int mb = static_cast<int>( TMaskBegin()(opState) & (typeBits-1) );
        unsigned int me = static_cast<int>( TMaskEnd()(opState) & (typeBits-1) );

        return static_cast<Type>(TSrc()(opState)) & PPC64GenMask<Type>(mb, me);
    }
};

// rldimi, rlwimi
template <typename Type, typename TSrc1, typename TSrc2, typename TMaskBegin, typename TMaskEnd>
struct PPC64MaskInsert : public std::unary_function<EmulatorUtility::OpEmulationState*, Type>
{
    Type operator()(EmulatorUtility::OpEmulationState* opState)
    {
        unsigned int typeBits = sizeof(Type)*8;
        unsigned int mb = static_cast<int>( TMaskBegin()(opState) & (typeBits-1) );
        unsigned int me = static_cast<int>( TMaskEnd()(opState) & (typeBits-1) );
        Type mask = PPC64GenMask<Type>(mb, me);

        return (static_cast<Type>(TSrc2()(opState)) & mask) | (static_cast<Type>(TSrc1()(opState)) & ~mask);
    }
};


// **********************************
//              compare
// **********************************

// Src1 と Src2 を比較し，結果を Condition Register の値として返す
template <typename Type, typename TSrc1, typename TSrc2>
struct PPC64Compare : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        return PPC64CalcFlag(static_cast<Type>(TSrc1()(opState)), static_cast<Type>(TSrc2()(opState)));
    }
};

template <typename Type, typename TSrc1, typename TSrc2>
struct PPC64FPCompare : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        return PPC64CalcFlagFP(static_cast<Type>(TSrc1()(opState)), static_cast<Type>(TSrc2()(opState)));
    }
};

// **********************************
//       conversion
// **********************************


// TSrc の浮動小数点数をTypeの符号付き整数型に変換する．
// Typeで表せる最大値を超えている場合は最大値を，最小値を下回っている場合は最小値を返す．
template <typename Type, typename TSrc, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
struct PPC64FPToInt : public std::unary_function<EmulatorUtility::OpEmulationState*, Type>
{
    Type operator()(EmulatorUtility::OpEmulationState* opState)
    {
        typedef typename EmulatorUtility::signed_type<Type>::type SignedType;
        typedef typename TSrc::result_type FPType;
        // 2の補数を仮定
        const SignedType maxValue = std::numeric_limits<SignedType>::max();
        const SignedType minValue = std::numeric_limits<SignedType>::min();
        FPType value = static_cast<FPType>( TSrc()(opState) );
        
        if (value > static_cast<FPType>(maxValue))
            return maxValue;
        else if (value < static_cast<FPType>(minValue))
            return minValue;
        else
            return static_cast<Type>(value);
    }
};

// floating round to integer nearest
template <typename TSrc>
struct PPC64FRIN : public std::unary_function<EmulatorUtility::OpEmulationState*, double>
{
    double operator()(EmulatorUtility::OpEmulationState* opState)
    {
        double value = static_cast<double>( TSrc()(opState) );

        if (value > 0.0)
            return floor(value+0.5);
        else
            return ceil(value-0.5);
    }
};

// floating round to integer zero
template <typename TSrc>
struct PPC64FRIZ : public std::unary_function<EmulatorUtility::OpEmulationState*, double>
{
    double operator()(EmulatorUtility::OpEmulationState* opState)
    {
        double value = static_cast<double>( TSrc()(opState) );

        if (value > 0.0)
            return floor(value);
        else
            return ceil(value);
    }
};

// floating round to integer plus
template <typename TSrc>
struct PPC64FRIP : public std::unary_function<EmulatorUtility::OpEmulationState*, double>
{
    double operator()(EmulatorUtility::OpEmulationState* opState)
    {
        double value = static_cast<double>( TSrc()(opState) );
        return ceil(value);
    }
};

// floating round to integer minus
template <typename TSrc>
struct PPC64FRIM : public std::unary_function<EmulatorUtility::OpEmulationState*, double>
{
    double operator()(EmulatorUtility::OpEmulationState* opState)
    {
        double value = static_cast<double>( TSrc()(opState) );
        return floor(value);
    }
};


// **********************************
//    CR logical operations
// **********************************
// func(TSrcCR1[TSrcCI1ビット目], TSrcCR2[TSrcCI2ビット目])
// インデックスはMSB=0
template <typename TSrcCR1, typename TSrcCI1, typename TSrcCR2, typename TSrcCI2>
struct PPC64CRAnd : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        return (shttl::test(TSrcCR1()(opState), 3-TSrcCI1()(opState)) & shttl::test(TSrcCR2()(opState), 3-TSrcCI2()(opState)));
    }
};
template <typename TSrcCR1, typename TSrcCI1, typename TSrcCR2, typename TSrcCI2>
struct PPC64CRNand : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        return (shttl::test(TSrcCR1()(opState), 3-TSrcCI1()(opState)) & shttl::test(TSrcCR2()(opState), 3-TSrcCI2()(opState))) ^ 1;
    }
};
template <typename TSrcCR1, typename TSrcCI1, typename TSrcCR2, typename TSrcCI2>
struct PPC64CROr : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        return (shttl::test(TSrcCR1()(opState), 3-TSrcCI1()(opState)) | shttl::test(TSrcCR2()(opState), 3-TSrcCI2()(opState)));
    }
};
template <typename TSrcCR1, typename TSrcCI1, typename TSrcCR2, typename TSrcCI2>
struct PPC64CRNor : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        return (shttl::test(TSrcCR1()(opState), 3-TSrcCI1()(opState)) | shttl::test(TSrcCR2()(opState), 3-TSrcCI2()(opState))) ^ 1;
    }
};
template <typename TSrcCR1, typename TSrcCI1, typename TSrcCR2, typename TSrcCI2>
struct PPC64CRXor : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        return (shttl::test(TSrcCR1()(opState), 3-TSrcCI1()(opState)) ^ shttl::test(TSrcCR2()(opState), 3-TSrcCI2()(opState)));
    }
};
template <typename TSrcCR1, typename TSrcCI1, typename TSrcCR2, typename TSrcCI2>
struct PPC64CREqv : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        return (shttl::test(TSrcCR1()(opState), 3-TSrcCI1()(opState)) ^ shttl::test(TSrcCR2()(opState), 3-TSrcCI2()(opState))) ^ 1;
    }
};
template <typename TSrcCR1, typename TSrcCI1, typename TSrcCR2, typename TSrcCI2>
struct PPC64CRAndC : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        return (shttl::test(TSrcCR1()(opState), 3-TSrcCI1()(opState)) | !shttl::test(TSrcCR2()(opState), 3-TSrcCI2()(opState)));
    }
};
template <typename TSrcCR1, typename TSrcCI1, typename TSrcCR2, typename TSrcCI2>
struct PPC64CROrC : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        return (shttl::test(TSrcCR1()(opState), 3-TSrcCI1()(opState)) | !shttl::test(TSrcCR2()(opState), 3-TSrcCI2()(opState)));
    }
};

// **********************************
//      shift operations
// **********************************

// carry of arithmetic right shift
template <typename Type, typename TSrc1, typename TSrc2, unsigned int count_mask>
struct PPC64CarryOfAShiftR : public std::unary_function<EmulatorUtility::OpEmulationState*, Type>
{
    Type operator()(EmulatorUtility::OpEmulationState* opState)
    {
        Type value = static_cast<Type>( TSrc1()(opState) );
        size_t count = static_cast<size_t>( TSrc2()(opState) ) & count_mask;
        
        if ((value & ((Type)1 << (sizeof(Type)*8-1)))   // 負であるか
            && (value & shttl::mask(0, std::min(count, sizeof(Type)*8)))    // シフトで'1'が捨てられるかどうか
            )
            return 1;
        else
            return 0;
    }
};


// **********************************
//    memory operations
// **********************************
template <typename Type, typename TAddrDest, typename TAddr>
struct PPC64LoadWithUpdate : public std::unary_function<EmulatorUtility::OpEmulationState*, Type>
{
    Type operator()(EmulatorUtility::OpEmulationState* opState)
    {
        u64 addr = TAddr()(opState);
        Type value = ReadMemory<Type>( opState, addr );

        TAddrDest::SetOperand( opState, addr );
        return value;
    }
};

template <typename Type, typename TAddrDest, typename TValue, typename TAddr>
inline void PPC64StoreWithUpdate(EmulatorUtility::OpEmulationState* opState)
{
    u64 addr = TAddr()(opState);
    WriteMemory<Type>(opState, addr, static_cast<Type>( TValue()(opState) ));
    TAddrDest::SetOperand( opState, addr );
}

// **********************************
//    flag operations
// **********************************
template <typename TSrcFlag, typename TSrcField, typename TSrcValue>
struct PPC64MTFSFI : public std::unary_function<EmulatorUtility::OpEmulationState*, u64 >
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        u64 fpscr = TSrcFlag()(opState);
        u64 value = TSrcValue()(opState) & shttl::mask(0, 4);
        int field = static_cast<int>(TSrcField()(opState));
        int bit_pos = 4*(7-field);
        u64 mask = ~shttl::mask(bit_pos, 4);

        return (fpscr & mask) | (value << bit_pos);
    }
};

template <typename TSrcFlag, typename TSrcFieldMask, typename TSrcValue>
struct PPC64MTFSF : public std::unary_function<EmulatorUtility::OpEmulationState*, u64 >
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        u64 fpscr = TSrcFlag()(opState);
        u64 value = TSrcValue()(opState) & shttl::mask(0, 32);
        int fieldMask = static_cast<int>(TSrcFieldMask()(opState));

        u64 mask = 0;
        for (int i = 0; i < 8; i ++) {
            if (fieldMask & shttl::mask(i, 1)) {
                mask |= shttl::mask(i*4, 4);
            }
        }
        mask = ~mask;

        return (fpscr & mask) | value;
    }
};


// **********************************
//    result setting
// **********************************

// TDestCR[TDestCIビット目] = TFunc()()
template <typename TDestCR, typename TDestCRI, typename TOrgCR, typename TFunc>
inline void PPC64SetCRBit(EmulatorUtility::OpEmulationState* opState)
{
    u64 value = TOrgCR()(opState);

    if (TFunc()(opState)) {
        value = shttl::set_bit(value, 3-TDestCRI()(opState));
    }
    else {
        value = shttl::reset_bit(value, 3-TDestCRI()(opState));
    }

    TDestCR::SetOperand(opState, value);
}

template <typename TDest, typename TFunc>
inline void PPC64SetFPSCR(EmulatorUtility::OpEmulationState* opState)
{
    typename TFunc::result_type result = PPC64AdjustFPSCR(TFunc()(opState));
    
    TDest::SetOperand(opState, result);
}

template <typename TDest, typename TDestFlag, typename TFunc>
inline void PPC64SetFPSCRF(EmulatorUtility::OpEmulationState* opState)
{
    typename TFunc::result_type result = PPC64AdjustFPSCR(TFunc()(opState));
    
    TDest::SetOperand(opState, result);
    TDestFlag::SetOperand(opState, result >> 27 & 0xf );
}

// TFuncの戻り値を TDest にセットし，戻り値に応じてTDestFlagにフラグを設定
template <typename TDest, typename TDestFlag, typename TFunc>
inline void PPC64SetF(EmulatorUtility::OpEmulationState* opState)
{
    typename TFunc::result_type result = TFunc()(opState);  // 結果のビット数を維持する

    TDest::SetOperand(opState, result);
    TDestFlag::SetOperand(opState, PPC64CalcFlag(EmulatorUtility::cast_to_signed(result)) );
}

// TFuncの戻り値を TDest にセットし，戻り値に応じてTDestFlagにフラグを設定
template <typename TDest, typename TDestFlag, typename TFPSCR, typename TFunc>
inline void PPC64SetFPF(EmulatorUtility::OpEmulationState* opState)
{
    typename TFunc::result_type result = TFunc()(opState);  // 結果のビット数を維持する

    TDest::SetOperand(opState, AsIntFunc<RegisterType>( result ));
    TDestFlag::SetOperand(opState, TFPSCR()(opState) >> 28 & 0xf );
}

// 符号拡張を行うSetF
template <typename TDest, typename TDestFlag, typename TFunc>
inline void PPC64SetSextF(EmulatorUtility::OpEmulationState* opState)
{
    typename EmulatorUtility::signed_type<typename TFunc::result_type>::type result = EmulatorUtility::cast_to_signed( TFunc()(opState) );  // 結果のビット数を維持する

    TDest::SetOperand(opState, result);
    TDestFlag::SetOperand(opState, PPC64CalcFlag(EmulatorUtility::cast_to_signed(result)) );
}

// **********************************
//    branch operations
// **********************************
//
// relative: current_pc + disp*sizeof(Code)
// 
// call saves next_pc to TDest
//
template <typename TSrcDisp>
inline void PPC64BranchRelUncond(EmulatorUtility::OpEmulationState* opState)
{
    u64 target = current_pc(opState) + 4*EmulatorUtility::cast_to_signed( TSrcDisp()(opState) );
    do_branch(opState, target);

    opState->SetTakenPC(target);
}

template <typename TSrcTarget>
inline void PPC64BranchAbsUncond(EmulatorUtility::OpEmulationState* opState)
{
    // <TODO> 0x03
    RegisterType addr = TSrcTarget()(opState) & ~(RegisterType)0x03;
    do_branch(opState, addr );

    opState->SetTakenPC(addr);
}

template <typename TSrcDisp, typename TCond>
inline void PPC64BranchRelCond(EmulatorUtility::OpEmulationState* opState)
{
    if ( TCond()(opState) ) {
        PPC64BranchRelUncond<TSrcDisp>(opState);
    }
    else {
        opState->SetTakenPC( current_pc(opState) + 4*EmulatorUtility::cast_to_signed( TSrcDisp()(opState) ));
    }
}

template <typename TSrcTarget, typename TCond>
inline void PPC64BranchAbsCond(EmulatorUtility::OpEmulationState* opState)
{
    if ( TCond()(opState) ) {
        PPC64BranchAbsUncond<TSrcTarget>(opState);
    }
    else {
        RegisterType addr = TSrcTarget()(opState) & ~(RegisterType)0x03;
        opState->SetTakenPC( addr );
    }
}


template <typename TDest, typename TSrcDisp>
inline void PPC64CallRelUncond(EmulatorUtility::OpEmulationState* opState)
{
    RegisterType ret_addr = static_cast<RegisterType>( next_pc(opState) );
    PPC64BranchRelUncond<TSrcDisp>(opState);
    TDest::SetOperand(opState, ret_addr );
}

template <typename TDest, typename TSrcTarget>
inline void PPC64CallAbsUncond(EmulatorUtility::OpEmulationState* opState)
{
    // <TODO> 0x03
    RegisterType ret_addr = static_cast<RegisterType>( next_pc(opState) );
    PPC64BranchAbsUncond<TSrcTarget>(opState);
    TDest::SetOperand(opState, ret_addr );
}

} // namespace Operation {
} // namespace PPC64Linux {
} // namespace Onikiri

#endif

