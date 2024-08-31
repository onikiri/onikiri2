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


#ifndef EMU_RISCV32LINUX_RISCV32_OPERATION_H
#define EMU_RISCV32LINUX_RISCV32_OPERATION_H

#include "SysDeps/fenv.h"
#include "SysDeps/STL/functional.h"
#include "Utility/RuntimeError.h"
#include "Emu/Utility/DecoderUtility.h"
#include "Emu/Utility/GenericOperation.h"
#include "Emu/Utility/System/Syscall/SyscallConvIF.h"
#include "Emu/Utility/System/Memory/MemorySystem.h"
#include "Emu/Utility/System/VirtualSystem.h"
#include "Emu/Utility/System/ProcessState.h"


namespace Onikiri {
namespace RISCV32Linux {
namespace Operation {

using namespace EmulatorUtility;
using namespace EmulatorUtility::Operation;

typedef u32 RISCV32RegisterType;

static const RISCV32RegisterType REG_VALUE_TRUE = 1;
static const RISCV32RegisterType REG_VALUE_FALSE = 0;

// Operands
template <int OperandIndex>
class RISCV32DstOperand
{
public:
    typedef RISCV32RegisterType type;
    static void SetOperand(OpEmulationState* opState, RISCV32RegisterType value)
    {
        opState->SetDst(OperandIndex, value);
    }
};

template <int OperandIndex>
struct RISCV32SrcOperand : public unary_function<OpEmulationState, RISCV32RegisterType>
{
    typedef RISCV32RegisterType type;
    RISCV32RegisterType operator()(OpEmulationState* opState)
    {
        return (RISCV32RegisterType)opState->GetSrc(OperandIndex);
    }
};

template <int OperandIndex>
struct RISCV32DstDoubleOperand
{
    using type = u64;
    static void SetOperand(OpEmulationState* opState, u64 value)
    {
        opState->SetDst(OperandIndex, value);
    }
};

template <typename Type, Type value>
struct RISCV32IntConst : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        return value;
    }
};


// interface
inline void RISCV32DoBranch(OpEmulationState* opState, u32 target)
{
    opState->SetTaken(true);
    opState->SetTakenPC(target);
}

inline u32 RISCV32NextPC(OpEmulationState* opState)
{
    return (u32)opState->GetPC() + 4;
}

inline u32 RISCV32CurPC(OpEmulationState* opState)
{
    return (u32)opState->GetPC();
}


// Set PC
template<typename TSrc1>
struct RISCV32Auipc : public unary_function<OpEmulationState, RegisterType>
{
    RISCV32RegisterType operator()(OpEmulationState* opState) const
    {
        return (TSrc1()(opState) << 12) + RISCV32CurPC(opState);
    }
};

// Load upper immediate
template<typename TSrc1>
struct RISCV32Lui : public unary_function<OpEmulationState, RegisterType>
{
    RISCV32RegisterType operator()(OpEmulationState* opState) const
    {
        return TSrc1()(opState) << 12;
    }
};


// compare
template <typename TSrc1, typename TSrc2, typename Comp>
struct RISCV32Compare : public unary_function<EmulatorUtility::OpEmulationState*, RISCV32RegisterType>
{
    RISCV32RegisterType operator()(EmulatorUtility::OpEmulationState* opState)
    {
        if (Comp()(TSrc1()(opState), TSrc2()(opState))) {
            return (RISCV32RegisterType)REG_VALUE_TRUE;
        }
        else {
            return (RISCV32RegisterType)REG_VALUE_FALSE;
        }
    }
};

template <typename Type, typename TSrc1, typename TSrc2>
struct RISCV32MIN : public unary_function<EmulatorUtility::OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>(TSrc1()(opState));
        Type rhs = static_cast<Type>(TSrc2()(opState));

        if (lhs > rhs) {
            return rhs;
        }
        else {
            return lhs;
        }
    }
};

template <typename Type, typename TSrc1, typename TSrc2>
struct RISCV32MAX : public unary_function<EmulatorUtility::OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>(TSrc1()(opState));
        Type rhs = static_cast<Type>(TSrc2()(opState));

        if (lhs < rhs) {
            return rhs;
        }
        else {
            return lhs;
        }
    }
};


// Branch
template <typename TSrcTarget, typename TSrcDisp>
inline void RISCV32BranchAbsUncond(OpEmulationState* opState)
{
    // The target address is obtained by adding the sign-extended 12-bit
    // I-immediate to the register rs1, then setting the least-significant
    // bit of the result to zero.
    // -- The RISC-V Instruction Set Manual Vol. 1: Unprivileged ISA
    //    Document Version 20190608-Base-Ratified p.21
    u32 mask = 0xfffffffe;
    u32 target = TSrcTarget()(opState) + cast_to_signed(TSrcDisp()(opState));
    RISCV32DoBranch(opState, target & mask);
}

template <typename TSrcDisp>
inline void RISCV32BranchRelUncond(OpEmulationState* opState)
{
    u32 target = RISCV32CurPC(opState) + cast_to_signed(TSrcDisp()(opState));
    RISCV32DoBranch(opState, target);
}

template <typename TSrcDisp, typename TCond>
inline void RISCV32BranchRelCond(OpEmulationState* opState)
{
    if (TCond()(opState)) {
        RISCV32BranchRelUncond<TSrcDisp>(opState);
    }
    else {
        opState->SetTakenPC(RISCV32NextPC(opState));
    }
}


template <typename TDest, typename TSrcDisp>
inline void RISCV32CallRelUncond(OpEmulationState* opState)
{
    RISCV32RegisterType ret = static_cast<RISCV32RegisterType>(RISCV32NextPC(opState));
    RISCV32BranchRelUncond<TSrcDisp>(opState);
    TDest::SetOperand(opState, ret);
}

template <typename TDest, typename TSrcTarget, typename TSrcDisp>
inline void RISCV32CallAbsUncond(OpEmulationState* opState)
{
    RISCV32RegisterType ret = static_cast<RISCV32RegisterType>(RISCV32NextPC(opState));
    RISCV32BranchAbsUncond<TSrcTarget, TSrcDisp>(opState);
    TDest::SetOperand(opState, ret);
}

//
// Load/Store
//
template<typename TSrc1, typename TSrc2>
struct RISCV32Addr : public unary_function<EmulatorUtility::OpEmulationState, RISCV32RegisterType>
{
    RISCV32RegisterType operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        return TSrc1()(opState) + EmulatorUtility::cast_to_signed(TSrc2()(opState));
    }
};

//
// Div/Rem
//
template <typename TSrc1, typename TSrc2>
struct RISCV32IntDiv : public unary_function<OpEmulationState*, u32>
{
    u32 operator()(OpEmulationState* opState)
    {
        s32 src1 = static_cast<s32>(TSrc1()(opState));
        s32 src2 = static_cast<s32>(TSrc2()(opState));
        if (src2 == 0){
            return static_cast<u32>(-1);
        }
        if (src1 < -0x7fffffff && src2 == -1) { // overflow
            return src1;
        }
        return static_cast<u32>(src1 / src2);
    }
};

template <typename TSrc1, typename TSrc2>
struct RISCV32IntRem : public unary_function<OpEmulationState*, u32>
{
    u32 operator()(OpEmulationState* opState)
    {
        s32 src1 = static_cast<s32>(TSrc1()(opState));
        s32 src2 = static_cast<s32>(TSrc2()(opState));
        if (src2 == 0){
            return src1;
        }
        if (src1 < -0x7fffffff && src2 == -1) { // overflow
            return 0;
        }
        return src1 % src2;
    }
};

template <typename TSrc1, typename TSrc2>
struct RISCV32IntDivu : public unary_function<OpEmulationState*, u32>
{
    u32 operator()(OpEmulationState* opState)
    {
        u32 src1 = static_cast<u32>(TSrc1()(opState));
        u32 src2 = static_cast<u32>(TSrc2()(opState));
        if (src2 == 0){
            return static_cast<u32>(-1);
        }
        return src1 / src2;
    }
};

template <typename TSrc1, typename TSrc2>
struct RISCV32IntRemu : public unary_function<OpEmulationState*, u32>
{
    u32 operator()(OpEmulationState* opState)
    {
        u32 src1 = static_cast<u32>(TSrc1()(opState));
        u32 src2 = static_cast<u32>(TSrc2()(opState));
        if (src2 == 0){
            return src1;
        }
        return src1 % src2;
    }
};


template <typename Type, typename TAddr>
struct RISCV32AtomicLoad : public unary_function<OpEmulationState*, u32>
{
    u32 operator()(OpEmulationState* opState)
    {
        MemorySystem* mem = opState->GetProcessState()->GetMemorySystem();
        MemAccess access;
        access.address.pid = opState->GetPID();
        access.address.address = TAddr()(opState);
        access.size = sizeof(Type);
        access.sign = true;

        mem->ReadMemory(&access);

        if (access.result != MemAccess::Result::MAR_SUCCESS) {
            THROW_RUNTIME_ERROR("Atomic memory read access failed: %s", access.ToString().c_str());
        }

        return (u32)access.value;
    }
};

template <typename Type, typename TValue, typename TAddr>
void RISCV32AtomicStore(OpEmulationState* opState)
{
    MemorySystem* mem = opState->GetProcessState()->GetMemorySystem();
    MemAccess access;
    access.address.pid = opState->GetPID();
    access.address.address = TAddr()(opState);
    access.size = sizeof(Type);
    access.value = TValue()(opState);

    mem->WriteMemory(&access);

    if (access.result != MemAccess::Result::MAR_SUCCESS) {
        THROW_RUNTIME_ERROR("Atomic memory write access failed: %s", access.ToString().c_str());
    }
}

namespace {
    // Virtual control status registers for holding the
    // reservation status of the 'load-reserved' instruction.
    enum class RISCV32pseudoCSR {
        // 0 - 4095 are used for RISCV32CSR (real CSRs)
        RESERVED_ADDRESS = 4096,
        RESERVING = 4097,
    };
}

// We cannot use the Load instead of the RISCV32AtomicLoad
// because atomic instructions are not iLD instructions,
// and this confuses the MemOrderManager.
//
// I assume it is sufficient that the processor remember
// only the reserved address. (or need the infomation of size?)
template <typename Type, typename TDest, typename TAddr>
void RISCV32LoadReserved(OpEmulationState* opState)
{
    Set<TDest, RISCV32AtomicLoad<Type, TAddr>>(opState);
    // Set reserved address
    ProcessState* process = opState->GetProcessState();
    process->SetControlRegister(static_cast<u32>(RISCV32pseudoCSR::RESERVED_ADDRESS), TAddr()(opState));
    process->SetControlRegister(static_cast<u32>(RISCV32pseudoCSR::RESERVING), 1);
}

template <typename Type, typename TDest, typename TValue, typename TAddr>
void RISCV32StoreConditional(OpEmulationState* opState)
{
    ProcessState* process = opState->GetProcessState();
    u32 reserved_addr = (u32)process->GetControlRegister(static_cast<u32>(RISCV32pseudoCSR::RESERVED_ADDRESS));
    u32 reserving = (u32)process->GetControlRegister(static_cast<u32>(RISCV32pseudoCSR::RESERVING));
    u32 addr = TAddr()(opState);
    if (reserving && reserved_addr == addr)
    {
        // We cannot use the Store instead of the RISCV32AtomicStore. See above.
        RISCV32AtomicStore<Type, TValue, TAddr>(opState);
        // 0 means success (always success; we assume only single thread execution)
        Set<TDest, IntConst<u32, 0>>(opState);
        // Release reserved address.
        process->SetControlRegister(static_cast<u32>(RISCV32pseudoCSR::RESERVING), 0);
    }
    else {
        // No store.

        // 1 means 'unspecified failure'.
        Set<TDest, IntConst<u32, 1>>(opState);
        // Don't release reserved address. Is this right?

    }
}

template<typename TSrc>
struct RISCV32NanBoxing : public unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        return 0xffffffff00000000 | AsIntFunc<u32, f32>(TSrc()(opState));
    }
};

template <typename Type, typename TSrc1, typename TSrc2, typename TSrc3, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
struct RISCV32MADD : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = TSrc1()(opState);
        Type rhs = TSrc2()(opState);
        Type ths = TSrc3()(opState);

        Onikiri::ScopedFESetRound sr(RoundMode()(opState));
        volatile Type rvalue = std::fma(lhs, rhs, ths);
        return rvalue;
    }
};

template <typename Type, typename TSrc1, typename TSrc2, typename TSrc3, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
struct RISCV32MSUB : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = TSrc1()(opState);
        Type rhs = TSrc2()(opState);
        Type ths = TSrc3()(opState);

        Onikiri::ScopedFESetRound sr(RoundMode()(opState));
        volatile Type rvalue = std::fma(lhs, rhs, -ths);
        return rvalue;
    }
};

template <typename Type, typename TSrc1, typename TSrc2, typename TSrc3, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
struct RISCV32NMADD : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = TSrc1()(opState);
        Type rhs = TSrc2()(opState);
        Type ths = TSrc3()(opState);

        Onikiri::ScopedFESetRound sr(RoundMode()(opState));
        volatile Type rvalue = std::fma(-lhs, rhs, -ths);
        return rvalue;
    }
};

template <typename Type, typename TSrc1, typename TSrc2, typename TSrc3, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
struct RISCV32NMSUB : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = TSrc1()(opState);
        Type rhs = TSrc2()(opState);
        Type ths = TSrc3()(opState);

        Onikiri::ScopedFESetRound sr(RoundMode()(opState));
        volatile Type rvalue = std::fma(-lhs, rhs, ths);
        return rvalue;
    }
};


template <typename T>
T CanonicalNAN();

template <>
inline f64 CanonicalNAN<f64>() {
    return AsFPFunc<f64, u64>(0x7ff8000000000000ull);
}

template <>
inline f32 CanonicalNAN<f32>() {
    return AsFPFunc<f32, u32>(0x7fc00000);
}

template <typename Type, typename TSrc1, typename TSrc2>
struct RISCV32FPMIN : public unary_function<EmulatorUtility::OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>(TSrc1()(opState));
        Type rhs = static_cast<Type>(TSrc2()(opState));

        // 双方 NaN の場合は正規化された NaN を返す
        if (std::isnan(lhs) && std::isnan(rhs)) {
            return CanonicalNAN<Type>();
        }

        // 符号付き負のゼロの方がが必ず小さくなるように
        if (lhs == 0.0 && rhs == 0.0) {
            return
                std::copysign(1, lhs) == (Type)-1.0 ||
                std::copysign(1, rhs) == (Type)-1.0 ? (Type)-0.0 : (Type)0.0;
        }

        // NaN じゃない方が常に選ばれるように
        if (std::isnan(lhs)) { lhs = std::numeric_limits<Type>::infinity(); }
        if (std::isnan(rhs)) { rhs = std::numeric_limits<Type>::infinity(); }
        return (lhs > rhs) ? rhs : lhs;
    }
};


template <typename Type, typename TSrc1, typename TSrc2>
struct RISCV32FPMAX : public unary_function<EmulatorUtility::OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>(TSrc1()(opState));
        Type rhs = static_cast<Type>(TSrc2()(opState));

        if (lhs == 0.0 && rhs == 0.0) {
            return
                std::copysign(1, lhs) == (Type)-1.0 &&
                std::copysign(1, rhs) == (Type)-1.0 ? (Type)-0.0 : (Type)0.0;
        }

        if (std::isnan(lhs) && std::isnan(rhs)) {
            return CanonicalNAN<Type>();
        }

        if (std::isnan(lhs)) { lhs = -std::numeric_limits<Type>::infinity(); }
        if (std::isnan(rhs)) { rhs = -std::numeric_limits<Type>::infinity(); }

        return (lhs < rhs) ? rhs : lhs;
    }
};


//conversion

namespace {
    // These functions define the min/max valid input, whose rounded result is representable in the destination format.
    // These min/max values may not be representable in the destination format, so we define the values in source format.
    template<typename FPType, typename IntType>
    FPType maximumValidInput();
    template<typename FPType, typename IntType>
    FPType minimumValidInput();

    template<> f32 maximumValidInput<f32, s32>() { return AsFPFunc<f32, u32>(0x4effffff); } // The max is  2^31 - 128, since 2^31-1 cannot be represented in f32.
    template<> f32 minimumValidInput<f32, s32>() { return AsFPFunc<f32, u32>(0xcf000000); } // The min is -2^31 because the next is -2^31 - 128, which is not representable in s32.
    template<> f32 maximumValidInput<f32, u32>() { return AsFPFunc<f32, u32>(0x4f7fffff); } // 2^32 - 256
    template<> f32 minimumValidInput<f32, u32>() { return AsFPFunc<f32, u32>(0xbf7fffff); } // The min is -0.99999994 that is the minimum number that rounded result is 0.
    template<> f64 maximumValidInput<f64, s32>() { return AsFPFunc<f64, u64>(0x41dfffffffffffff); } //  2^31-1 + 0.9999998
    template<> f64 minimumValidInput<f64, s32>() { return AsFPFunc<f64, u64>(0xc1e00000001fffff); } // -2^31   - 0.9999998
    template<> f64 maximumValidInput<f64, u32>() { return AsFPFunc<f64, u64>(0x41efffffffffffff); } //  2^32-1 + 0.9999995
    template<> f64 minimumValidInput<f64, u32>() { return AsFPFunc<f64, u64>(0xbfefffffffffffff); } // -0.9999999999999999
}

// Convert floating point number TSrc to integer Type.
template <typename Type, typename TSrc, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
struct RISCV32FPToInt : public unary_function<EmulatorUtility::OpEmulationState*, Type>
{
    Type operator()(EmulatorUtility::OpEmulationState* opState)
    {
        typedef typename TSrc::result_type FPType;

        constexpr Type maxValue = std::numeric_limits<Type>::max();
        constexpr Type minValue = std::numeric_limits<Type>::min();
        FPType value = TSrc()(opState);

        if (std::isnan(value)) // NaN
            return maxValue;
        else if (std::isinf(value) && !std::signbit(value)) // +Inf
            return maxValue;
        else if (std::isinf(value) && std::signbit(value)) // -Inf
            return minValue;
        else if (value > maximumValidInput<FPType, Type>())
            return maxValue;
        else if (value < minimumValidInput<FPType, Type>())
            return minValue;
        else {
            Onikiri::ScopedFESetRound sr(RoundMode()(opState));
            // std::nearbyint rounds argument to an integral value, using the rounding direction specified by fegetround.
            volatile Type ret = static_cast<Type>(std::nearbyint(value));
            return ret;
        }
    }
};


template <typename TSrc1, typename TSrc2>
struct FPFLOATCopySign : public unary_function<OpEmulationState*, u32>
{
    u32 operator()(OpEmulationState* opState)
    {
        u32 lhs = static_cast<u32>(TSrc1()(opState));
        u32 rhs = static_cast<u32>(TSrc2()(opState));

        return (lhs & (u32)0x80000000) | (rhs & ~(u32)0x80000000);
    }
};


template <typename TSrc1, typename TSrc2>
struct FPFLOATCopySignNeg : public unary_function<OpEmulationState*, u32>
{
    u32 operator()(OpEmulationState* opState)
    {
        u32 lhs = static_cast<u32>(TSrc1()(opState));
        u32 rhs = static_cast<u32>(TSrc2()(opState));

        return (~lhs & (u32)0x80000000) | (rhs & ~(u32)0x80000000);
    }
};

template <typename TSrc1, typename TSrc2>
struct FPDoubleCopySignXor : public unary_function<OpEmulationState*, u64>
{
    u64 operator()(OpEmulationState* opState)
    {
        u64 lhs = static_cast<u64>(TSrc1()(opState));
        u64 rhs = static_cast<u64>(TSrc2()(opState));

        return (lhs & (u64)0x8000000000000000ULL) ^ rhs;
    }
};


template <typename TSrc1, typename TSrc2>
struct FPFLOATCopySignXor : public unary_function<OpEmulationState*, u64>
{
    u32 operator()(OpEmulationState* opState)
    {
        u32 lhs = static_cast<u32>(TSrc1()(opState));
        u32 rhs = static_cast<u32>(TSrc2()(opState));

        return (lhs & (u32)0x80000000) ^ rhs;
    }
};

//compare

template <typename Type, typename TSrc1, typename TSrc2>
struct RISCV32FPEqual : public unary_function<EmulatorUtility::OpEmulationState*, bool>
{
    bool operator()(EmulatorUtility::OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>(TSrc1()(opState));
        Type rhs = static_cast<Type>(TSrc2()(opState));

        return lhs == rhs;
    }
};

template <typename Type, typename TSrc1, typename TSrc2>
struct RISCV32FPLess : public unary_function<EmulatorUtility::OpEmulationState*, bool>
{
    bool operator()(EmulatorUtility::OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>(TSrc1()(opState));
        Type rhs = static_cast<Type>(TSrc2()(opState));

        return lhs < rhs;
    }
};

template <typename Type, typename TSrc1, typename TSrc2>
struct RISCV32FPLessEqual : public unary_function<EmulatorUtility::OpEmulationState*, bool>
{
    bool operator()(EmulatorUtility::OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>(TSrc1()(opState));
        Type rhs = static_cast<Type>(TSrc2()(opState));

        return lhs <= rhs;
    }
};


// 指数部が全て1で，仮数部が非ゼロなら NaN
// さらに，仮数部の最上位が 0 なら Signaling
inline bool IsSignalingNAN(f64 fpValue) {
    u64 intValue = AsIntFunc<u64>(fpValue);
    return
        (((intValue >> 51) & 0xFFF) == 0xFFE) &&
        (intValue & 0x7FFFFFFFFFFFFull);
}

inline bool IsSignalingNAN(f32 fpValue) {
    u32 intValue = AsIntFunc<u32>(fpValue);
    return
        (((intValue >> 22) & 0x1FF) == 0x1FE) &&
        (intValue & 0x3FFFFF);
}

template <typename Type, typename TSrc1>
struct RISCV32FCLASS : public unary_function<EmulatorUtility::OpEmulationState*, u32>
{
    u32 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        Type value = static_cast<Type>(TSrc1()(opState));

        switch(std::fpclassify(value)) {
        case FP_INFINITE:
            return std::signbit(value) ? (u32)(1 << 0) : (u32)(1 << 7);
        case FP_NAN:
            return IsSignalingNAN(value) ? (u32)(1 << 8) : (u32)(1 << 9);
        case FP_NORMAL:
            return std::signbit(value) ? (u32)(1 << 1) : (u32)(1 << 6);
        case FP_SUBNORMAL:
            return std::signbit(value) ? (u32)(1 << 2) : (u32)(1 << 5);
        case FP_ZERO:
            return std::signbit(value) ? (u32)(1 << 3) : (u32)(1 << 4);
        default:
            return 0;
        }
    }
};



//
// CSR Operations
//
namespace {
    enum class RISCV32CSR
    {
        FFLAGS   = 0x001,
        FRM      = 0x002,
        FCSR     = 0x003,
        CYCLE    = 0xC00,
        TIME     = 0xC01,
        INSTRET  = 0xC02,
        CYCLEH   = 0xC80,
        TIMEH    = 0xC81,
        INSTRETH = 0xC82,
    };

    enum class RISCV32FRM
    {
        RNE = 0, // Round to Nearest, ties to Even
        RTZ = 1, // Round towards Zero
        RDN = 2, // Round Down (towards −infinity)
        RUP = 3, // Round Up (towards +infinity)
        RMM = 4, // Round to Nearest, ties to Max Magnitude
    };

    u32 GetCSR_Value(OpEmulationState* opState, RISCV32CSR csrNum)
    {
        ProcessState* process = opState->GetProcessState();
        switch (csrNum) {
        case RISCV32CSR::FFLAGS:
        case RISCV32CSR::FRM:
            return (u32)process->GetControlRegister(static_cast<u32>(csrNum));

        case RISCV32CSR::FCSR:
            return
                (u32)(
                    process->GetControlRegister(static_cast<u32>(RISCV32CSR::FFLAGS)) |
                    (process->GetControlRegister(static_cast<u32>(RISCV32CSR::FRM)) << 5)
                );

        case RISCV32CSR::CYCLE:
        case RISCV32CSR::TIME:
        case RISCV32CSR::INSTRET:
            return (u32)process->GetVirtualSystem()->GetInsnTick();
        case RISCV32CSR::CYCLEH:
        case RISCV32CSR::TIMEH:
        case RISCV32CSR::INSTRETH:
            return (u32)(process->GetVirtualSystem()->GetInsnTick() >> 32);
        default:
            RUNTIME_WARNING("Unimplemented CSR is read: %d", static_cast<int>(csrNum));
            return (u32)process->GetControlRegister(static_cast<u32>(csrNum));
        }
    }

    void SetCSR_Value(OpEmulationState* opState, RISCV32CSR csrNum, u32 value)
    {
        ProcessState* process = opState->GetProcessState();
        switch (csrNum) {
        case RISCV32CSR::FFLAGS:
            process->SetControlRegister(static_cast<u32>(csrNum), ExtractBits(value, 0, 5));
            break;

        case RISCV32CSR::FRM:
            process->SetControlRegister(static_cast<u32>(csrNum), ExtractBits(value, 0, 2));
            break;

        case RISCV32CSR::FCSR:  // Map to FFLAGS/FRM
            process->SetControlRegister(static_cast<u32>(RISCV32CSR::FFLAGS), ExtractBits(value, 0, 5));
            process->SetControlRegister(static_cast<u32>(RISCV32CSR::FRM), ExtractBits(value, 5, 3));
            break;

        // These registers are read-only
        case RISCV32CSR::CYCLE:
        case RISCV32CSR::TIME:
        case RISCV32CSR::INSTRET:
        case RISCV32CSR::CYCLEH:
        case RISCV32CSR::TIMEH:
        case RISCV32CSR::INSTRETH:

        default:
            RUNTIME_WARNING("Unimplemented CSR is written: %d", static_cast<int>(csrNum));
            process->SetControlRegister(static_cast<u32>(csrNum), value);
        }
    }

} // namespace `anonymous'

template <typename TDest, typename TSrc1, typename CSR_S>
void RISCV32CSRRW(OpEmulationState* opState)
{
    u32 srcValue = static_cast<u32>(SrcOperand<0>()(opState));
    RISCV32CSR csrNum = (RISCV32CSR)CSR_S()(opState);
    u32 csrValue = GetCSR_Value(opState, csrNum);
    SetCSR_Value(opState, csrNum, srcValue);
    TDest::SetOperand(opState, csrValue);
}

template <typename TDest, typename TSrc1, typename CSR_S>
void RISCV32CSRRS(OpEmulationState* opState)
{
    u32 srcValue = static_cast<u32>(SrcOperand<0>()(opState));
    RISCV32CSR csrNum = (RISCV32CSR)CSR_S()(opState);
    u32 csrValue = GetCSR_Value(opState, csrNum);
    SetCSR_Value(opState, csrNum, csrValue | srcValue);
    TDest::SetOperand(opState, csrValue);    // Return the original value
}

template <typename TDest, typename TSrc1, typename CSR_S>
void RISCV32CSRRC(OpEmulationState* opState)
{
    u32 srcValue = static_cast<u32>(SrcOperand<0>()(opState));
    RISCV32CSR csrNum = (RISCV32CSR)CSR_S()(opState);
    u32 csrValue = GetCSR_Value(opState, csrNum);
    SetCSR_Value(opState, csrNum, csrValue & (~srcValue));
    TDest::SetOperand(opState, csrValue);    // Return the original value
}

// Return a current rounding mode specified by FRM
struct RISCV32RoundModeFromFCSR : public unary_function<EmulatorUtility::OpEmulationState, u32>
{
    int operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        u32 frm = GetCSR_Value(opState, RISCV32CSR::FRM);
        switch (static_cast<RISCV32FRM>(frm)) {
        case RISCV32FRM::RNE:
            return FE_TONEAREST;
        case RISCV32FRM::RTZ:
            return FE_TOWARDZERO;
        case RISCV32FRM::RDN:
            return FE_DOWNWARD;
        case RISCV32FRM::RUP:
            return FE_UPWARD;
        case RISCV32FRM::RMM:
            return FE_TONEAREST;
        default:
            RUNTIME_WARNING("Undefined rounding mode: %d", (int)frm);
            return FE_TONEAREST;
        }
    }
};


void RISCV32SyscallSetArg(EmulatorUtility::OpEmulationState* opState)
{
    EmulatorUtility::SyscallConvIF* syscallConv = opState->GetProcessState()->GetSyscallConv();
    syscallConv->SetArg(0, SrcOperand<0>()(opState));
    syscallConv->SetArg(1, SrcOperand<1>()(opState));
    syscallConv->SetArg(2, SrcOperand<2>()(opState));
    DstOperand<0>::SetOperand(opState, SrcOperand<0>()(opState));
}

void RISCV32SyscallSetArg2(EmulatorUtility::OpEmulationState* opState)
{
    EmulatorUtility::SyscallConvIF* syscallConv = opState->GetProcessState()->GetSyscallConv();
    syscallConv->SetArg(3, SrcOperand<1>()(opState));
    syscallConv->SetArg(4, SrcOperand<2>()(opState));
}

// invoke syscall, get result&error and branch if any
void RISCV32SyscallCore(EmulatorUtility::OpEmulationState* opState)
{
    EmulatorUtility::SyscallConvIF* syscallConv = opState->GetProcessState()->GetSyscallConv();
    syscallConv->SetArg(5, SrcOperand<1>()(opState));
    syscallConv->Execute(opState);

    u32 error = (u32)syscallConv->GetResult(EmulatorUtility::SyscallConvIF::ErrorFlagIndex);
    u32 val = (u32)syscallConv->GetResult(EmulatorUtility::SyscallConvIF::RetValueIndex);
    DstOperand<0>::SetOperand(opState, error ? (u32)-1 : val);
}

} // namespace Operation {
} // namespace RISCV32Linux {
} // namespace Onikiri

#endif // #ifndef EMU_ALPHA_LINUX_ALPHALINUX_ALPHAOPERATION_H

