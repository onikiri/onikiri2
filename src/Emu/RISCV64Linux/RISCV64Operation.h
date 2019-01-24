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


#ifndef EMU_RISCV64LINUX_RISCV64_OPERATION_H
#define EMU_RISCV64LINUX_RISCV64_OPERATION_H

#include "SysDeps/fenv.h"
#include "Utility/RuntimeError.h"
#include "Emu/Utility/DecoderUtility.h"
#include "Emu/Utility/GenericOperation.h"
#include "Emu/Utility/System/Syscall/SyscallConvIF.h"
#include "Emu/Utility/System/ProcessState.h"
#include "Emu/Utility/System/Memory/MemorySystem.h"
#include "Emu/Utility/System/VirtualSystem.h"


namespace Onikiri {
    namespace RISCV64Linux {
        namespace Operation {

            using namespace EmulatorUtility;
            using namespace EmulatorUtility::Operation;

            typedef u64 RISCV64RegisterType;

            static const RISCV64RegisterType REG_VALUE_TRUE = 1;
            static const RISCV64RegisterType REG_VALUE_FALSE = 0;

            // Operands
            template <int OperandIndex>
            class RISCV64DstOperand
            {
            public:
                typedef RISCV64RegisterType type;
                static void SetOperand(OpEmulationState* opState, RISCV64RegisterType value)
                {
                    opState->SetDst(OperandIndex, value);
                }
            };

            template <int OperandIndex>
            struct RISCV64SrcOperand : public std::unary_function<OpEmulationState, RISCV64RegisterType>
            {
                typedef RISCV64RegisterType type;
                RISCV64RegisterType operator()(OpEmulationState* opState)
                {
                    return (RISCV64RegisterType)opState->GetSrc(OperandIndex);
                }
            };

            template <typename Type, Type value>
            struct RISCV64IntConst : public std::unary_function<OpEmulationState*, Type>
            {
                Type operator()(OpEmulationState* opState)
                {
                    return value;
                }
            };


            // interface
            inline void RISCV64DoBranch(OpEmulationState* opState, u64 target)
            {
                opState->SetTaken(true);
                opState->SetTakenPC(target);
            }

            inline u64 RISCV64NextPC(OpEmulationState* opState)
            {
                return (u64)opState->GetPC() + 4;
            }

            inline u64 RISCV64CurPC(OpEmulationState* opState)
            {
                return (u64)opState->GetPC();
            }


            // Set PC
            template<typename TSrc1>
            struct RISCV64Auipc : public std::unary_function<OpEmulationState, RegisterType>
            {
                RISCV64RegisterType operator()(OpEmulationState* opState) const
                {
                    return (TSrc1()(opState) << 12) + RISCV64CurPC(opState);
                }
            };

            // Load upper immediate
            template<typename TSrc1>
            struct RISCV64Lui : public std::unary_function<OpEmulationState, RegisterType>
            {
                RISCV64RegisterType operator()(OpEmulationState* opState) const
                {
                    return TSrc1()(opState) << 12;
                }
            };


            // compare
            template <typename TSrc1, typename TSrc2, typename Comp>
            struct RISCV64Compare : public std::unary_function<EmulatorUtility::OpEmulationState*, RISCV64RegisterType>
            {
                RISCV64RegisterType operator()(EmulatorUtility::OpEmulationState* opState)
                {
                    if (Comp()(TSrc1()(opState), TSrc2()(opState))) {
                        return (RISCV64RegisterType)REG_VALUE_TRUE;
                    }
                    else {
                        return (RISCV64RegisterType)REG_VALUE_FALSE;
                    }
                }
            };


            template <typename Type, typename TSrc1, typename TSrc2>
            struct RISCV64MIN : public std::unary_function<EmulatorUtility::OpEmulationState*, Type>
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
            struct RISCV64MAX : public std::unary_function<EmulatorUtility::OpEmulationState*, Type>
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

            template <typename T>
            T CanonicalNAN() {
                return 0;
            }

            template <>
            inline f64 CanonicalNAN<f64>() {
                return AsFPFunc<f64, u64>(0x7ff8000000000000ull);
            }

            template <>
            inline f32 CanonicalNAN<f32>() {
                return AsFPFunc<f32, u32>(0x7fc00000);
            }

            template <typename Type, typename TSrc1, typename TSrc2>
            struct RISCV64FPMIN : public std::unary_function<EmulatorUtility::OpEmulationState*, Type>
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
            struct RISCV64FPMAX : public std::unary_function<EmulatorUtility::OpEmulationState*, Type>
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
            
            //
            // FP operations
            //

            template<typename TSrc>
            struct RISCV64NanBoxing : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
            {
                u64 operator()(EmulatorUtility::OpEmulationState* opState) const
                {
                    return 0xffffffff00000000 | AsIntFunc<u32, f32>(TSrc()(opState));
                }
            };

            template <typename Type, typename TSrc1, typename TSrc2, typename TSrc3, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
            struct RISCV64MADD : public std::unary_function<OpEmulationState*, Type>
            {
                Type operator()(OpEmulationState* opState)
                {
                    Type lhs = TSrc1()(opState);
                    Type rhs = TSrc2()(opState);
                    Type ths = TSrc3()(opState); //変数名適当です

                    Onikiri::ScopedFESetRound sr(RoundMode()(opState));
                    volatile Type rvalue = std::fma(lhs, rhs, ths);
                    return rvalue;
                }
            };

            template <typename Type, typename TSrc1, typename TSrc2, typename TSrc3, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
            struct RISCV64MSUB : public std::unary_function<OpEmulationState*, Type>
            {
                Type operator()(OpEmulationState* opState)
                {
                    Type lhs = TSrc1()(opState);
                    Type rhs = TSrc2()(opState);
                    Type ths = TSrc3()(opState); //変数名適当です

                    Onikiri::ScopedFESetRound sr(RoundMode()(opState));
                    volatile Type rvalue = std::fma(lhs, rhs, -ths);
                    return rvalue;
                }
            };

            template <typename Type, typename TSrc1, typename TSrc2, typename TSrc3, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
            struct RISCV64NMADD : public std::unary_function<OpEmulationState*, Type>
            {
                Type operator()(OpEmulationState* opState)
                {
                    Type lhs = TSrc1()(opState);
                    Type rhs = TSrc2()(opState);
                    Type ths = TSrc3()(opState); //変数名適当です

                    Onikiri::ScopedFESetRound sr(RoundMode()(opState));
                    volatile Type rvalue = std::fma(-lhs, rhs, -ths);
                    return rvalue;
                }
            };

            template <typename Type, typename TSrc1, typename TSrc2, typename TSrc3, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
            struct RISCV64NMSUB : public std::unary_function<OpEmulationState*, Type>
            {
                Type operator()(OpEmulationState* opState)
                {
                    Type lhs = TSrc1()(opState);
                    Type rhs = TSrc2()(opState);
                    Type ths = TSrc3()(opState); //変数名適当です

                    Onikiri::ScopedFESetRound sr(RoundMode()(opState));
                    volatile Type rvalue = std::fma(-lhs, rhs, ths);
                    return rvalue;
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
                template<> f32 maximumValidInput<f32, s64>() { return AsFPFunc<f32, u32>(0x5effffff); } // 2^63 - 2^39
                template<> f32 minimumValidInput<f32, s64>() { return AsFPFunc<f32, u32>(0xdf000000); } // -2^63
                template<> f32 maximumValidInput<f32, u64>() { return AsFPFunc<f32, u32>(0x5f7fffff); } // 2^64 - 2^40
                template<> f32 minimumValidInput<f32, u64>() { return AsFPFunc<f32, u32>(0xbf7fffff); } // -0.99999994
                template<> f64 maximumValidInput<f64, s32>() { return AsFPFunc<f64, u64>(0x41dfffffffffffff); } //  2^31-1 + 0.9999998
                template<> f64 minimumValidInput<f64, s32>() { return AsFPFunc<f64, u64>(0xc1e00000001fffff); } // -2^31   - 0.9999998
                template<> f64 maximumValidInput<f64, u32>() { return AsFPFunc<f64, u64>(0x41efffffffffffff); } //  2^32-1 + 0.9999995
                template<> f64 minimumValidInput<f64, u32>() { return AsFPFunc<f64, u64>(0xbfefffffffffffff); } // -0.9999999999999999
                template<> f64 maximumValidInput<f64, s64>() { return AsFPFunc<f64, u64>(0x43dfffffffffffff); } //  2^63 - 2048
                template<> f64 minimumValidInput<f64, s64>() { return AsFPFunc<f64, u64>(0xc3e0000000000000); } // -2^63
                template<> f64 maximumValidInput<f64, u64>() { return AsFPFunc<f64, u64>(0x43efffffffffffff); } //  2^64 - 4096
                template<> f64 minimumValidInput<f64, u64>() { return AsFPFunc<f64, u64>(0xbfefffffffffffff); } // -0.9999999999999999
            }


            // Convert floating point number TSrc to integer Type.
            template <typename Type, typename TSrc, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
            struct RISCV64FPToInt : public std::unary_function<EmulatorUtility::OpEmulationState*, Type>
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
                        volatile Type ret = static_cast<Type>(value);
                        return ret;
                    }
                }
            };


            // sign from src1 and the rest from src2
            //FLOAT版、GenericOperationに入れてもいいかも
            template <typename TSrc1, typename TSrc2>
            struct FPFLOATCopySign : public std::unary_function<OpEmulationState*, u64>
            {
                u32 operator()(OpEmulationState* opState)
                {
                    u32 lhs = static_cast<u32>(TSrc1()(opState));
                    u32 rhs = static_cast<u32>(TSrc2()(opState));

                    return (lhs & (u32)0x80000000) | (rhs & ~(u32)0x80000000);
                }
            };


            template <typename TSrc1, typename TSrc2>
            struct FPFLOATCopySignNeg : public std::unary_function<OpEmulationState*, u64>
            {
                u32 operator()(OpEmulationState* opState)
                {
                    u32 lhs = static_cast<u32>(TSrc1()(opState));
                    u32 rhs = static_cast<u32>(TSrc2()(opState));

                    return (~lhs & (u32)0x80000000) | (rhs & ~(u32)0x80000000);
                }
            };

            template <typename TSrc1, typename TSrc2>
            struct FPDoubleCopySignXor : public std::unary_function<OpEmulationState*, u64>
            {
                u64 operator()(OpEmulationState* opState)
                {
                    u64 lhs = static_cast<u64>(TSrc1()(opState));
                    u64 rhs = static_cast<u64>(TSrc2()(opState));

                    return (lhs & (u64)0x8000000000000000ULL) ^ rhs;
                }
            };


            template <typename TSrc1, typename TSrc2>
            struct FPFLOATCopySignXor : public std::unary_function<OpEmulationState*, u64>
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
            struct RISCV64FPEqual : public std::unary_function<EmulatorUtility::OpEmulationState*, bool>
            {
                bool operator()(EmulatorUtility::OpEmulationState* opState)
                {
                    Type lhs = static_cast<Type>(TSrc1()(opState));
                    Type rhs = static_cast<Type>(TSrc2()(opState));

                    return lhs == rhs;
                }
            };

            template <typename Type, typename TSrc1, typename TSrc2>
            struct RISCV64FPLess : public std::unary_function<EmulatorUtility::OpEmulationState*, bool>
            {
                bool operator()(EmulatorUtility::OpEmulationState* opState)
                {
                    Type lhs = static_cast<Type>(TSrc1()(opState));
                    Type rhs = static_cast<Type>(TSrc2()(opState));

                    return lhs < rhs;
                }
            };

            template <typename Type, typename TSrc1, typename TSrc2>
            struct RISCV64FPLessEqual : public std::unary_function<EmulatorUtility::OpEmulationState*, bool>
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

            //ここを頑張る
            template <typename Type, typename TSrc1>
            struct RISCV64FCLASS : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
            {
                u64 operator()(EmulatorUtility::OpEmulationState* opState)
                {
                    Type value = static_cast<Type>(TSrc1()(opState));

                    switch(std::fpclassify(value)) {
                    case FP_INFINITE:
                        return std::signbit(value) ? (u64)(1 << 0) : (u64)(1 << 7);
                    case FP_NAN:
                        return IsSignalingNAN(value) ? (u64)(1 << 8) : (u64)(1 << 9);
                    case FP_NORMAL:
                        return std::signbit(value) ? (u64)(1 << 1) : (u64)(1 << 6);
                    case FP_SUBNORMAL:
                        return std::signbit(value) ? (u64)(1 << 2) : (u64)(1 << 5);
                    case FP_ZERO:
                        return std::signbit(value) ? (u64)(1 << 3) : (u64)(1 << 4);
                    default:
                        return 0;
                    }
                }
            };


            // Branch
            template <typename TSrcTarget, typename TSrcDisp>
            inline void RISCV64BranchAbsUncond(OpEmulationState* opState)
            {
                u64 target = TSrcTarget()(opState) + cast_to_signed(TSrcDisp()(opState));
                RISCV64DoBranch(opState, target);
            }

            template <typename TSrcDisp>
            inline void RISCV64BranchRelUncond(OpEmulationState* opState)
            {
                u64 target = RISCV64CurPC(opState) + cast_to_signed(TSrcDisp()(opState));
                RISCV64DoBranch(opState, target);
            }

            template <typename TSrcDisp, typename TCond>
            inline void RISCV64BranchRelCond(OpEmulationState* opState)
            {
                if (TCond()(opState)) {
                    RISCV64BranchRelUncond<TSrcDisp>(opState);
                }
                else {
                    opState->SetTakenPC(RISCV64NextPC(opState));
                }
            }


            template <typename TDest, typename TSrcDisp>
            inline void RISCV64CallRelUncond(OpEmulationState* opState)
            {
                RISCV64RegisterType ret = static_cast<RISCV64RegisterType>(RISCV64NextPC(opState));
                RISCV64BranchRelUncond<TSrcDisp>(opState);
                TDest::SetOperand(opState, ret);
            }

            template <typename TDest, typename TSrcTarget, typename TSrcDisp>
            inline void RISCV64CallAbsUncond(OpEmulationState* opState)
            {
                RISCV64RegisterType ret = static_cast<RISCV64RegisterType>(RISCV64NextPC(opState));
                RISCV64BranchAbsUncond<TSrcTarget, TSrcDisp>(opState);
                TDest::SetOperand(opState, ret);
            }

            //
            // Load/Store
            //
            template<typename TSrc1, typename TSrc2>
            struct RISCV64Addr : public std::unary_function<EmulatorUtility::OpEmulationState, RISCV64RegisterType>
            {
                RISCV64RegisterType operator()(EmulatorUtility::OpEmulationState* opState) const
                {
                    return TSrc1()(opState) + EmulatorUtility::cast_to_signed(TSrc2()(opState));
                }
            };

            //
            // Div/Rem
            //
            template <typename TSrc1, typename TSrc2>
            struct RISCV64IntDiv : public std::unary_function<OpEmulationState*, u64>
            {
                u64 operator()(OpEmulationState* opState)
                {
                    s64 src1 = static_cast<s64>(TSrc1()(opState));
                    s64 src2 = static_cast<s64>(TSrc2()(opState));
                    if (src2 == 0) {
                        return static_cast<u64>(-1);
                    }
                    if (src1 < -0x7fffffffffffffff && src2 == -1) { // overflow
                        return src1;
                    }
                    return static_cast<u64>(src1 / src2);
                }
            };

            template <typename TSrc1, typename TSrc2>
            struct RISCV64IntRem : public std::unary_function<OpEmulationState*, u64>
            {
                u64 operator()(OpEmulationState* opState)
                {
                    s64 src1 = static_cast<s64>(TSrc1()(opState));
                    s64 src2 = static_cast<s64>(TSrc2()(opState));
                    if (src2 == 0) {
                        return src1;
                    }
                    if (src1 < -0x7fffffffffffffff && src2 == -1) { // overflow
                        return 0;
                    }
                    return src1 % src2;
                }
            };

            template <typename TSrc1, typename TSrc2>
            struct RISCV64IntDivu : public std::unary_function<OpEmulationState*, u64>
            {
                u64 operator()(OpEmulationState* opState)
                {
                    u64 src1 = static_cast<u64>(TSrc1()(opState));
                    u64 src2 = static_cast<u64>(TSrc2()(opState));
                    if (src2 == 0) {
                        return static_cast<u64>(-1);
                    }
                    return src1 / src2;
                }
            };

            template <typename TSrc1, typename TSrc2>
            struct RISCV64IntRemu : public std::unary_function<OpEmulationState*, u64>
            {
                u64 operator()(OpEmulationState* opState)
                {
                    u64 src1 = static_cast<u64>(TSrc1()(opState));
                    u64 src2 = static_cast<u64>(TSrc2()(opState));
                    if (src2 == 0) {
                        return src1;
                    }
                    return src1 % src2;
                }
            };

            template <typename TSrc1, typename TSrc2>
            struct RISCV64IntDivw : public std::unary_function<OpEmulationState*, u32>
            {
                u32 operator()(OpEmulationState* opState)
                {
                    s32 src1 = static_cast<s32>(TSrc1()(opState));
                    s32 src2 = static_cast<s32>(TSrc2()(opState));
                    if (src2 == 0) {
                        return static_cast<u32>(-1);
                    }
                    if (src1 < -0x7fffffff && src2 == -1) { // overflow
                        return src1;
                    }
                    return static_cast<u32>(src1 / src2);
                }
            };

            template <typename TSrc1, typename TSrc2>
            struct RISCV64IntRemw : public std::unary_function<OpEmulationState*, u32>
            {
                u32 operator()(OpEmulationState* opState)
                {
                    s32 src1 = static_cast<s32>(TSrc1()(opState));
                    s32 src2 = static_cast<s32>(TSrc2()(opState));
                    if (src2 == 0) {
                        return src1;
                    }
                    if (src1 < -0x7fffffff && src2 == -1) { // overflow
                        return 0;
                    }
                    return src1 % src2;
                }
            };

            template <typename TSrc1, typename TSrc2>
            struct RISCV64IntDivuw : public std::unary_function<OpEmulationState*, u32>
            {
                u32 operator()(OpEmulationState* opState)
                {
                    u32 src1 = static_cast<u32>(TSrc1()(opState));
                    u32 src2 = static_cast<u32>(TSrc2()(opState));
                    if (src2 == 0) {
                        return static_cast<u32>(-1);
                    }
                    return src1 / src2;
                }
            };

            template <typename TSrc1, typename TSrc2>
            struct RISCV64IntRemuw : public std::unary_function<OpEmulationState*, u32>
            {
                u32 operator()(OpEmulationState* opState)
                {
                    u32 src1 = static_cast<u32>(TSrc1()(opState));
                    u32 src2 = static_cast<u32>(TSrc2()(opState));
                    if (src2 == 0) {
                        return src1;
                    }
                    return src1 % src2;
                }
            };

            template <typename Type, typename TAddr>
            struct RISCV64AtomicLoad : public std::unary_function<OpEmulationState*, u64>
            {
                u64 operator()(OpEmulationState* opState)
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

                    return access.value;
                }
            };

            template <typename Type, typename TValue, typename TAddr>
            void RISCV64AtomicStore(OpEmulationState* opState)
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
                enum class RISCV64pseudoCSR {
                    // 0 - 4095 are used for RISCV64CSR (real CSRs)
                    RESERVED_ADDRESS = 4096,
                    RESERVING = 4097,
                };
            }

            // We cannot use the Load instead of the RISCV64AtomicLoad
            // because atomic instructions are not iLD instructions,
            // and this confuses the MemOrderManager.
            //
            // I assume it is sufficient that the processor remember
            // only the reserved address. (or need the infomation of size?)
            template <typename Type, typename TDest, typename TAddr>
            void RISCV64LoadReserved(OpEmulationState* opState)
            {
                Set<TDest, RISCV64AtomicLoad<Type, TAddr>>(opState);
                // Set reserved address
                ProcessState* process = opState->GetProcessState();
                process->SetControlRegister(static_cast<u64>(RISCV64pseudoCSR::RESERVED_ADDRESS), TAddr()(opState));
                process->SetControlRegister(static_cast<u64>(RISCV64pseudoCSR::RESERVING), 1);
            }

            template <typename Type, typename TDest, typename TValue, typename TAddr>
            void RISCV64StoreConditional(OpEmulationState* opState)
            {
                ProcessState* process = opState->GetProcessState();
                u64 reserved_addr = process->GetControlRegister(static_cast<u64>(RISCV64pseudoCSR::RESERVED_ADDRESS));
                u64 reserving = process->GetControlRegister(static_cast<u64>(RISCV64pseudoCSR::RESERVING));
                u64 addr = TAddr()(opState);
                if (reserving && reserved_addr == addr)
                {
                    // We cannot use the Store instead of the RISCV64AtomicStore. See above.
                    RISCV64AtomicStore<Type, TValue, TAddr>(opState);
                    // 0 means success (always success; we assume only single thread execution)
                    Set<TDest, IntConst<u64, 0>>(opState);
                    // Release reserved address.
                    process->SetControlRegister(static_cast<u64>(RISCV64pseudoCSR::RESERVING), 0);
                }
                else {
                    // No store.

                    // 1 means 'unspecified failure'.
                    Set<TDest, IntConst<u64, 1>>(opState);
                    // Don't release reserved address. Is this right?

                }
            }

            void RISCV64SyscallSetArg(EmulatorUtility::OpEmulationState* opState)
            {
                EmulatorUtility::SyscallConvIF* syscallConv = opState->GetProcessState()->GetSyscallConv();
                syscallConv->SetArg(0, SrcOperand<0>()(opState));
                syscallConv->SetArg(1, SrcOperand<1>()(opState));
                syscallConv->SetArg(2, SrcOperand<2>()(opState));
                // Make dependency between this op and the next op so that 
                // they are executed in-order
                DstOperand<0>::SetOperand(opState, SrcOperand<0>()(opState));
            }

            // invoke syscall, get result&error and branch if any
            void RISCV64SyscallCore(EmulatorUtility::OpEmulationState* opState)
            {
                EmulatorUtility::SyscallConvIF* syscallConv = opState->GetProcessState()->GetSyscallConv();
                syscallConv->SetArg(3, SrcOperand<1>()(opState));
                syscallConv->SetArg(4, SrcOperand<2>()(opState));
                //syscallConv->SetArg(5, SrcOperand<2>()(opState));
                syscallConv->Execute(opState);

                u64 error = (u64)syscallConv->GetResult(EmulatorUtility::SyscallConvIF::ErrorFlagIndex);
                u64 val = (u64)syscallConv->GetResult(EmulatorUtility::SyscallConvIF::RetValueIndex);
                
                // When error occurs, an error number is coded into a register 
                // in the range [-4095, -1] in RISCV linux
                if (error) {
                    if (val == 0)
                        val = (u64)-1;
                    else
                        val = (u64)(-((s64)val));
                }

                DstOperand<0>::SetOperand(opState, val);
            }


            //
            // CSR Operations
            //
            namespace {
                enum class RISCV64CSR
                {
                    FFLAGS  = 0x001,
                    FRM     = 0x002,
                    FCSR    = 0x003,
                    CYCLE   = 0xC00,
                    TIME    = 0xC01,
                    INSTRET = 0xC02,
                };

                enum class RISCV64FRM
                {
                    RNE = 0, // Round to Nearest, ties to Even
                    RTZ = 1, // Round towards Zero
                    RDN = 2, // Round Down (towards −infinity)
                    RUP = 3, // Round Up (towards +infinity)
                    RMM = 4, // Round to Nearest, ties to Max Magnitude
                };

                /*
                const char* CSR_NumToStr(RISCV64CSR csr)
                {
                    switch (csr){
                    case RISCV64CSR::FFLAGS : return "FFLAGS";
                    case RISCV64CSR::FRM    : return "FRM";
                    case RISCV64CSR::FCSR   : return "FCSR";
                    case RISCV64CSR::CYCLE  : return "CYCLE";
                    case RISCV64CSR::TIME   : return "TIME";
                    case RISCV64CSR::INSTRET: return "INSTRET";
                    default                 : return "<invalid>";
                    }
                }*/

                u64 GetCSR_Value(OpEmulationState* opState, RISCV64CSR csrNum)
                {
                    ProcessState* process = opState->GetProcessState();
                    switch (csrNum) {
                    case RISCV64CSR::FFLAGS:
                    case RISCV64CSR::FRM:
                        return process->GetControlRegister(static_cast<u64>(csrNum));

                    case RISCV64CSR::FCSR:
                        return
                            process->GetControlRegister(static_cast<u64>(RISCV64CSR::FFLAGS)) |
                            (process->GetControlRegister(static_cast<u64>(RISCV64CSR::FRM)) << 5);

                    case RISCV64CSR::CYCLE:
                    case RISCV64CSR::TIME:
                    case RISCV64CSR::INSTRET:
                        return process->GetVirtualSystem()->GetInsnTick();
                    default:
                        RUNTIME_WARNING("Unimplemented CSR is read: %d", static_cast<int>(csrNum));
                        return process->GetControlRegister(static_cast<u64>(csrNum));
                    }
                }
                
                void SetCSR_Value(OpEmulationState* opState, RISCV64CSR csrNum, u64 value)
                {
                    ProcessState* process = opState->GetProcessState();
                    switch (csrNum) {
                    case RISCV64CSR::FFLAGS:
                        process->SetControlRegister(static_cast<u64>(csrNum), ExtractBits(value, 0, 5));
                        break;

                    case RISCV64CSR::FRM:
                        process->SetControlRegister(static_cast<u64>(csrNum), ExtractBits(value, 0, 2));
                        break;

                    case RISCV64CSR::FCSR:  // Map to FFLAGS/FRM
                        process->SetControlRegister(static_cast<u64>(RISCV64CSR::FFLAGS), ExtractBits(value, 0, 5));
                        process->SetControlRegister(static_cast<u64>(RISCV64CSR::FRM), ExtractBits(value, 5, 3));
                        break;

                    // These registers are read-only
                    case RISCV64CSR::CYCLE:
                    case RISCV64CSR::TIME:
                    case RISCV64CSR::INSTRET:
                        break;

                    default:
                        RUNTIME_WARNING("Unimplemented CSR is written: %d", static_cast<int>(csrNum));
                        process->SetControlRegister(static_cast<u64>(csrNum), value);
                    }
                }

            } // namespace `anonymous'

            template <typename TDest, typename TSrc1, typename CSR_S>
            inline void RISCV64CSRRW(OpEmulationState* opState)
            {
                u64 srcValue = static_cast<u64>(SrcOperand<0>()(opState));
                RISCV64CSR csrNum = (RISCV64CSR)CSR_S()(opState);
                u64 csrValue = GetCSR_Value(opState, csrNum);
                SetCSR_Value(opState, csrNum, srcValue);
                TDest::SetOperand(opState, csrValue);
            }

            template <typename TDest, typename TSrc1, typename CSR_S>
            inline void RISCV64CSRRS(OpEmulationState* opState)
            {
                u64 srcValue = static_cast<u64>(SrcOperand<0>()(opState));
                RISCV64CSR csrNum = (RISCV64CSR)CSR_S()(opState);
                u64 csrValue = GetCSR_Value(opState, csrNum);
                SetCSR_Value(opState, csrNum, csrValue | srcValue);
                TDest::SetOperand(opState, csrValue);    // Return the original value
            }

            template <typename TDest, typename TSrc1, typename CSR_S>
            inline void RISCV64CSRRC(OpEmulationState* opState)
            {
                u64 srcValue = static_cast<u64>(SrcOperand<0>()(opState));
                RISCV64CSR csrNum = (RISCV64CSR)CSR_S()(opState);
                u64 csrValue = GetCSR_Value(opState, csrNum);
                SetCSR_Value(opState, csrNum, csrValue & (~srcValue));
                TDest::SetOperand(opState, csrValue);    // Return the original value
            }
            
            // Return a current rounding mode specified by FRM
            struct RISCV64RoundModeFromFCSR : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
            {
                int operator()(EmulatorUtility::OpEmulationState* opState) const
                {
                    u64 frm = GetCSR_Value(opState, RISCV64CSR::FRM);
                    switch (static_cast<RISCV64FRM>(frm)) {
                    case RISCV64FRM::RNE:
                        return FE_TONEAREST;
                    case RISCV64FRM::RTZ:
                        return FE_TOWARDZERO;
                    case RISCV64FRM::RDN:
                        return FE_DOWNWARD;
                    case RISCV64FRM::RUP:
                        return FE_UPWARD;
                    case RISCV64FRM::RMM:
                        return FE_TONEAREST;
                    default:
                        RUNTIME_WARNING("Undefined rounding mode: %d", (int)frm);
                        return FE_TONEAREST;
                    }
                }
            };


        } // namespace Operation {
    } // namespace RISCV64Linux {
} // namespace Onikiri

#endif // #ifndef EMU_ALPHA_LINUX_ALPHALINUX_ALPHAOPERATION_H
