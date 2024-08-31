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


#ifndef EMU_UTILITY_EMULATORUTILITY_GENERIC_OPERATION_H
#define EMU_UTILITY_EMULATORUTILITY_GENERIC_OPERATION_H

#include <cstring>

#include "SysDeps/fenv.h"
#include "SysDeps/STL/functional.h"
#include "Emu/Utility/Math.h"
#include "Emu/Utility/OpEmulationState.h"

namespace Onikiri {
namespace EmulatorUtility {
namespace Operation {

// その他関数

// high-order 64 bits of the 128-bit product of unsigned lhs and rhs
u64 UnsignedMulHigh64(u64 lhs, u64 rhs);
// high-order 64 bits of the 128-bit product of signed lhs and rhs
s64 SignedMulHigh64(s64 lhs, s64 rhs);
// high-order 64 bits of the 128-bit product of signed lhs and unsigned rhs
s64 SignedUnsignedMulHigh64(s64 lhs, u64 rhs);

typedef u64 RegisterType;

// Destには必ず代入すること
// Destに書かないことはDestの値が保持されることを意味しない（Destは新規に作成されたPhyRegなのでゴミが入っている）

// interface
inline void do_branch(OpEmulationState* opState, u64 target)
{
    opState->SetTaken(true);
}

inline u64 next_pc(OpEmulationState* opState)
{
    return opState->GetPC()+4;
}

inline u64 current_pc(OpEmulationState* opState)
{
    return opState->GetPC();
}

template <typename T>
inline void WriteMemory(OpEmulationState* opState, u64 addr, const T &value)
{
    Onikiri::MemAccess access;
    access.address.pid = opState->GetPID();
    access.address.address = addr;
    access.size = sizeof(T);
    access.sign = false;
    access.value = value;
    opState->GetOpState()->Write( &access );
}

template <typename T>
inline T ReadMemory(OpEmulationState* opState, u64 addr)
{
    Onikiri::MemAccess access;
    access.address.pid = opState->GetPID();
    access.address.address = addr;
    access.size = sizeof(T);
    access.sign = false;
    access.value = 0;
    opState->GetOpState()->Read( &access );
    return static_cast<T>( access.value );
}


// utility

// 整数型のvalueを，同じビット表現を持つ浮動小数点型に変換する
template <typename FPType, typename IntType>
inline FPType AsFPFunc(IntType value)
{
    BOOST_STATIC_ASSERT(sizeof(IntType) == sizeof(FPType));

    // ビット列の再解釈にはunionを用いる方法が有名だが、
    // unionを用いたビット列再解釈はC言語では正しいがC++では誤り
    // （strict aliasing rules に違反している未定義動作）
    // 2010年代以降のコンパイラでは単なるレジスタ転送命令に
    // 最適化されるため、C言語でunionを用いたコードと同程度の性能となる
    FPType f;
    std::memcpy(&f, &value, sizeof(f));
    return f;
}

// 浮動小数点型のvalueを，同じビット表現を持つ整数型に変換する
template <typename IntType, typename FPType>
inline IntType AsIntFunc(FPType value)
{
    BOOST_STATIC_ASSERT(sizeof(IntType) == sizeof(FPType));
    // unionを用いるのは誤り。AsFPFuncのコメントを参照。
    IntType i;
    std::memcpy(&i, &value, sizeof(i));
    return i;
}

// 整数のビット列を浮動小数点数として再解釈 ( reinterpret as fp )
template <typename Type, typename TSrc>
struct AsFP : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        return AsFPFunc<Type>( TSrc()(opState) );
    }
};

// 浮動小数点のビット列を整数として再解釈 ( reinterpret as int )
template <typename Type, typename TSrc>
struct AsInt : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        return AsIntFunc<Type>( TSrc()(opState) );
    }
};

// **********************************
// operands
// **********************************

// operand はunsignedの値を返すこと

template <int OperandIndex>
class DstOperand
{
public:
    typedef RegisterType type;
    static void SetOperand(OpEmulationState* opState, RegisterType value)
    {
        opState->SetDst(OperandIndex, value);
    }
};

template <int OperandIndex>
struct SrcOperand : public unary_function<OpEmulationState, RegisterType>
{
    typedef RegisterType type;
    RegisterType operator()(OpEmulationState* opState)
    {
        return opState->GetSrc(OperandIndex);
    }
};

template <typename Type, Type value>
struct IntConst : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        return value;
    }
};

// **********************************
// conditions
// **********************************

//
// integer conditions
//
template<typename Type, typename ArgType = RegisterType>
struct IntCondEqual : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return static_cast<Type>(lhs) == static_cast<Type>(rhs);
    }
};

template<typename Type, typename ArgType = RegisterType>
struct IntCondNotEqual : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return static_cast<Type>(lhs) != static_cast<Type>(rhs);
    }
};

template<typename Type, typename ArgType = RegisterType>
struct IntCondLessSigned : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return cast_to_signed(static_cast<Type>(lhs)) < cast_to_signed(static_cast<Type>(rhs));
    }
};

template<typename Type, typename ArgType = RegisterType>
struct IntCondLessUnsigned : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return cast_to_unsigned(static_cast<Type>(lhs)) < cast_to_unsigned(static_cast<Type>(rhs));
    }
};

template<typename Type, typename ArgType = RegisterType>
struct IntCondLessEqualSigned : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return cast_to_signed(static_cast<Type>(lhs)) <= cast_to_signed(static_cast<Type>(rhs));
    }
};

template<typename Type, typename ArgType = RegisterType>
struct IntCondLessEqualUnsigned : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return cast_to_unsigned(static_cast<Type>(lhs)) <= cast_to_unsigned(static_cast<Type>(rhs));
    }
};

template<typename Type, typename ArgType = RegisterType>
struct IntCondGreaterSigned : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return cast_to_signed(static_cast<Type>(lhs)) > cast_to_signed(static_cast<Type>(rhs));
    }
};

template<typename Type, typename ArgType = RegisterType>
struct IntCondGreaterUnsigned : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return cast_to_unsigned(static_cast<Type>(lhs)) > cast_to_unsigned(static_cast<Type>(rhs));
    }
};

template<typename Type, typename ArgType = RegisterType>
struct IntCondGreaterEqualSigned : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return cast_to_signed(static_cast<Type>(lhs)) >= cast_to_signed(static_cast<Type>(rhs));
    }
};

template<typename Type, typename ArgType = RegisterType>
struct IntCondGreaterEqualUnsigned : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return cast_to_unsigned(static_cast<Type>(lhs)) >= cast_to_unsigned(static_cast<Type>(rhs));
    }
};

template<typename Type, int n, typename ArgType = RegisterType>
struct IntCondEqualNthBit : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return (static_cast<Type>(lhs) & ((Type)1 << n)) == (static_cast<Type>(rhs) & ((Type)1 << n));
    }
};

template<typename Type, int n, typename ArgType = RegisterType>
struct IntCondNotEqualNthBit : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return (static_cast<Type>(lhs) & ((Type)1 << n)) != (static_cast<Type>(rhs) & ((Type)1 << n));
    }
};

//
// fp conditions
//
template<typename Type, typename ArgType = RegisterType>
struct FPCondEqual : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return AsFPFunc<Type>(lhs) == AsFPFunc<Type>(rhs);
    }
};

template<typename Type, typename ArgType = RegisterType>
struct FPCondNotEqual : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return AsFPFunc<Type>(lhs) != AsFPFunc<Type>(rhs);
    }
};

template<typename Type, typename ArgType = RegisterType>
struct FPCondLess : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return AsFPFunc<Type>(lhs) < AsFPFunc<Type>(rhs);
    }
};

template<typename Type, typename ArgType = RegisterType>
struct FPCondLessEqual : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return AsFPFunc<Type>(lhs) <= AsFPFunc<Type>(rhs);
    }
};

template<typename Type, typename ArgType = RegisterType>
struct FPCondGreater : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return AsFPFunc<Type>(lhs) > AsFPFunc<Type>(rhs);
    }
};

template<typename Type, typename ArgType = RegisterType>
struct FPCondGreaterEqual : public binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return AsFPFunc<Type>(lhs) >= AsFPFunc<Type>(rhs);
    }
};

template <typename TSrc1, typename TSrc2>
struct CondAnd : public unary_function<OpEmulationState*, bool>
{
    bool operator()(OpEmulationState* opState)
    {
        bool lhs = TSrc1()(opState) != 0;
        bool rhs = TSrc2()(opState) != 0;

        return lhs && rhs;
    }
};

template <typename TSrc1, typename TSrc2>
struct CondOr : public unary_function<OpEmulationState*, bool>
{
    bool operator()(OpEmulationState* opState)
    {
        bool lhs = TSrc1()(opState) != 0;
        bool rhs = TSrc2()(opState) != 0;

        return lhs || rhs;
    }
};

// **********************************
// operations
// **********************************
inline void UndefinedOperation(OpEmulationState* opState)
{
    // do nothing
    // opState->undef = true ?
}


inline void NoOperation(OpEmulationState* opState)
{
    // no operation
}

// 関数Func1, Func2, ... を単に連続して呼ぶ
template <
    void (*Func1)(OpEmulationState *),
    void (*Func2)(OpEmulationState *)
>
inline void Sequence2(OpEmulationState* opState)
{
    Func1(opState);
    Func2(opState);
}

template <
    void (*Func1)(OpEmulationState *),
    void (*Func2)(OpEmulationState *),
    void (*Func3)(OpEmulationState *)
>
inline void Sequence3(OpEmulationState* opState)
{
    Func1(opState);
    Func2(opState);
    Func3(opState);
}

// **********************************
//    result setting
// **********************************
template <typename TDest, typename TFunc>
inline void SetSext(OpEmulationState* opState)
{
    TDest::SetOperand(opState, cast_to_signed( TFunc()(opState) ) );
}

template <typename TDest, typename TFunc>
inline void Set(OpEmulationState* opState)
{
    TDest::SetOperand(opState, TFunc()(opState) );
}

template <typename TDest, typename TFunc>
inline void SetFP(OpEmulationState* opState)
{
    TDest::SetOperand(opState, AsIntFunc<RegisterType>( TFunc()(opState) ) );
}

template <typename Type, typename TCond, typename TTrueValue, typename TFalseValue>
struct Select : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        if ( TCond()(opState) ) {
            return TTrueValue()(opState);
        }
        else {
            return TFalseValue()(opState);
        }
    }
};

// TValue を Type に変換する
template <typename Type, typename TValue>
struct Cast : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        return static_cast<Type>( TValue()(opState) );
    }
};

// 丸めモードを指定して TValue を Type に変換する．
template <typename Type, typename TValue, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
struct CastFP : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Onikiri::ScopedFESetRound sr( RoundMode()(opState) );
        volatile Type rvalue = static_cast<Type>( TValue()(opState) );
        return rvalue;
    }
};

// **********************************
//    memory operations
// **********************************
template <typename Type, typename TAddr>
struct Load : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        return ReadMemory<Type>( opState, TAddr()(opState) );
    }
};

template <typename Type, typename TValue, typename TAddr>
inline void Store(OpEmulationState* opState)
{
    WriteMemory<Type>(opState, TAddr()(opState), static_cast<Type>( TValue()(opState) ));
}

// Sets the value TFunc to TDest and returns the same value.
// The return type is the same as TFunc type (not TDest type).
template <typename TDest, typename TFunc>
struct TeeSet // : public unary_function<OpEmulationState*, decltype(TFunc()(static_cast<OpEmulationState*>(nullptr)))> // unary_function is deprecated in C++11. Don't use.
{
    auto operator()(OpEmulationState* opState) -> decltype(TFunc()(opState))
    {
        auto value = TFunc()(opState);
        TDest::SetOperand(opState, value);
        return value;
    }
};

// Sets the sign extended value TFunc to TDest and returns the same value.
// The return type is the signed type of TFunc type (not the TDest type).
template <typename TDest, typename TFunc>
struct TeeSetSext // : public unary_function<OpEmulationState*, decltype(TFunc()(static_cast<OpEmulationState*>(nullptr)))> // unary_function is deprecated in C++11. Don't use.
{
    auto operator()(OpEmulationState* opState) -> decltype(TFunc()(opState))
    {
        auto value = cast_to_signed(TFunc()(opState));
        TDest::SetOperand(opState, value);
        return value;
    }
};

// **********************************
//    int arithmetic operations
// **********************************

template <typename Type, typename TSrc1, typename TSrc2>
struct IntAdd : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        return static_cast<Type>(TSrc1()(opState)) + static_cast<Type>(TSrc2()(opState));
    }
};

template <typename Type, typename TSrc1, typename TSrc2>
struct IntSub : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        return static_cast<Type>(TSrc1()(opState)) - static_cast<Type>(TSrc2()(opState));
    }
};

template <typename Type, typename TSrc1, typename TSrc2>
struct IntMul : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        return static_cast<Type>(TSrc1()(opState)) * static_cast<Type>(TSrc2()(opState));
    }
};

template <typename Type, typename TSrc1, typename TSrc2>
struct IntDiv : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type src2 = static_cast<Type>(TSrc2()(opState));
        if( src2 == 0 ){
            u64 addr = opState->GetOpState()->GetPC().address;
            RUNTIME_WARNING( 
                "Division by zero occurred and returned 0 at PC: %08x%08x", 
                (u32)(addr >> 32),
                (u32)(addr & 0xffffffff)
            );
            return 0;
        }
        return static_cast<Type>(TSrc1()(opState)) / static_cast<Type>(TSrc2()(opState));
    }
};

// Src1 を shift_countだけ左シフトして加算
template <typename Type, int shift_count, typename TSrc1, typename TSrc2>
struct IntScaledAdd : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        return (static_cast<Type>(TSrc1()(opState)) << shift_count) + static_cast<Type>(TSrc2()(opState));
    }
};

// Src1 を shift_countだけ左シフトして減算
template <typename Type, int shift_count, typename TSrc1, typename TSrc2>
struct IntScaledSub : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        return (static_cast<Type>(TSrc1()(opState)) << shift_count) - static_cast<Type>(TSrc2()(opState));
    }
};

// compare
template <typename TSrc1, typename TSrc2, typename Comp>
struct Compare : public unary_function<OpEmulationState*, bool>
{
    bool operator()(OpEmulationState* opState)
    {
        return Comp()(TSrc1()(opState), TSrc2()(opState));
    }
};

// negate
template <typename Type, typename TSrc1>
struct IntNeg : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        return cast_to_unsigned( -cast_to_signed( static_cast<Type>( TSrc1()(opState) ) ) );
    }
};

// Type must be unsigned
template <typename Type, typename TSrc1, typename TSrc2>
struct CarryOfAdd : public unary_function<OpEmulationState*, RegisterType>
{
    RegisterType operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>( TSrc1()(opState) );
        Type rhs = static_cast<Type>( TSrc2()(opState) );

        if (lhs + rhs < rhs)
            return 1;
        else
            return 0;
    }
};

// Type must be unsigned
template <typename Type, typename TSrc1, typename TSrc2, typename TSrcCarry>
struct CarryOfAddWithCarry : public unary_function<OpEmulationState*, RegisterType>
{
    RegisterType operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>( TSrc1()(opState) );
        Type rhs = static_cast<Type>( TSrc2()(opState) );
        Type carry = static_cast<Type>( TSrcCarry()(opState) );

        ASSERT(carry == 0 || carry == 1);

        if (lhs + rhs < rhs || lhs + rhs + carry < rhs) // lhs, rhs がともに~0の場合 lhs + rhs + 1 < rhs は成立しない (ex. 255 + 255 + 1 = 255)
            return 1;
        else
            return 0;
    }
};

// Type must be unsigned
template <typename Type, typename TSrc1, typename TSrc2>
struct BorrowOfSub : public unary_function<OpEmulationState*, RegisterType>
{
    RegisterType operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>( TSrc1()(opState) );
        Type rhs = static_cast<Type>( TSrc2()(opState) );

        if (lhs < rhs)
            return 1;
        else
            return 0;
    }
};

// Type must be unsigned
template <typename Type, typename TSrc1, typename TSrc2, typename TSrcBorrow>
struct BorrowOfSubWithBorrow : public unary_function<OpEmulationState*, RegisterType>
{
    RegisterType operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>( TSrc1()(opState) );
        Type rhs = static_cast<Type>( TSrc2()(opState) );
        Type borrow = static_cast<Type>( TSrcBorrow()(opState) );

        ASSERT(borrow == 0 || borrow == 1);

        if (rhs == ~(Type)0 || lhs < rhs + borrow)
            return 1;
        else
            return 0;
    }
};

template <typename TSrc1, typename TSrc2>
struct IntUMulh64 : public unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(OpEmulationState* opState) const
    {
        u64 lhs = TSrc1()(opState);
        u64 rhs = TSrc2()(opState);

        return UnsignedMulHigh64(lhs, rhs);
    }
};

template <typename TSrc1, typename TSrc2>
struct IntSMulh64 : public unary_function<EmulatorUtility::OpEmulationState, u64>
{
    s64 operator()(OpEmulationState* opState) const
    {
        s64 lhs = static_cast<s64>(TSrc1()(opState));
        s64 rhs = TSrc2()(opState);

        return SignedMulHigh64(lhs, rhs);
    }
};

template <typename TSrc1, typename TSrc2>
struct IntSUMulh64 : public unary_function<EmulatorUtility::OpEmulationState, u64>
{
	s64 operator()(OpEmulationState* opState) const
	{
		s64 lhs = static_cast<s64>(TSrc1()(opState));
		u64 rhs = static_cast<s64>(TSrc2()(opState));

		return SignedUnsignedMulHigh64(lhs, rhs);
	}
};

template <typename Type1, typename Type2, typename TSrc1, typename TSrc2>
struct IntUMulh32 : public unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u32 operator()(OpEmulationState* opState) const
    {
        Type1 lhs = TSrc1()(opState);
        Type2 rhs = TSrc2()(opState);
        Type1 ret = static_cast<Type1>(lhs * rhs) >> 32;
        return static_cast<u32>(ret);
    }
};

// **********************************
//    shift operations
// **********************************

// arithmetic right shift
template <typename Type, typename TSrc1, typename TSrc2, unsigned int count_mask>
struct AShiftR : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type value = static_cast<Type>( TSrc1()(opState) );
        typename TSrc2::result_type count = static_cast<typename TSrc2::result_type>( TSrc2()(opState) ) & count_mask;
        
        if (count >= sizeof(Type)*8)
            return 0;
        else
            return static_cast<Type>( cast_to_unsigned( cast_to_signed(value) >> count ) );
    }
};

// logical right shift
template <typename Type, typename TSrc1, typename TSrc2, unsigned int count_mask>
struct LShiftR : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type value = static_cast<Type>( TSrc1()(opState) );
        typename TSrc2::result_type count = static_cast<typename TSrc2::result_type>( TSrc2()(opState) ) & count_mask;
        
        if (count >= sizeof(Type)*8)
            return 0;
        else
            return static_cast<Type>( cast_to_unsigned( cast_to_unsigned(value) >> count ) );
    }
};
// logical left shift
template <typename Type, typename TSrc1, typename TSrc2, unsigned int count_mask>
struct LShiftL : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type value = static_cast<Type>( TSrc1()(opState) );
        typename TSrc2::result_type count = static_cast<typename TSrc2::result_type>( TSrc2()(opState) ) & count_mask;
        
        if (count >= sizeof(Type)*8)
            return 0;
        else
            return static_cast<Type>( cast_to_unsigned( cast_to_unsigned(value) << count ) );
    }
};

// rotate left
template <typename Type, typename TSrcValue, typename TSrcCount>
struct RotateL : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        const int typeBitCount = sizeof(Type)*8;
        Type value = static_cast<Type>( TSrcValue()(opState) );
        int count = static_cast<int>( TSrcCount()(opState) & (typeBitCount-1) );

        if (count == 0)
            return value;
        else
            return (value << count) | (value >> (typeBitCount-count));
    }
};

template <typename Type, typename TSrcValue, typename TSrcCount>
struct RotateR : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        const int typeBitCount = sizeof(Type)*8;
        Type value = static_cast<Type>( TSrcValue()(opState) );
        int count = static_cast<int>( TSrcCount()(opState) & (typeBitCount-1) );

        if (count == 0)
            return value;
        else
            return (value >> count) | (value << (typeBitCount-count));
    }
};


// **********************************
//    logical operations
// **********************************

// and, or, xor, ... はC++の予約語 (代替表記)

// src1 & src2
template <typename Type, typename TSrc1, typename TSrc2>
struct BitAnd : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>( TSrc1()(opState) );
        Type rhs = static_cast<Type>( TSrc2()(opState) );

        return lhs & rhs;
    }
};


// src1 & ~src2
template <typename Type, typename TSrc1, typename TSrc2>
struct BitAndNot : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>( TSrc1()(opState) );
        Type rhs = static_cast<Type>( TSrc2()(opState) );

        return lhs & ~rhs;
    }
};

// ~(src1 & src2)
template <typename Type, typename TSrc1, typename TSrc2>
struct BitNand : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>( TSrc1()(opState) );
        Type rhs = static_cast<Type>( TSrc2()(opState) );

        return ~(lhs & rhs);
    }
};

// src1 | src2
template <typename Type, typename TSrc1, typename TSrc2>
struct BitOr : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>( TSrc1()(opState) );
        Type rhs = static_cast<Type>( TSrc2()(opState) );

        return lhs | rhs;
    }
};

// src1 | ~src2
template <typename Type, typename TSrc1, typename TSrc2>
struct BitOrNot : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>( TSrc1()(opState) );
        Type rhs = static_cast<Type>( TSrc2()(opState) );

        return lhs | ~rhs;
    }
};

// ~(src1 | src2)
template <typename Type, typename TSrc1, typename TSrc2>
struct BitNor : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>( TSrc1()(opState) );
        Type rhs = static_cast<Type>( TSrc2()(opState) );

        return ~(lhs | rhs);
    }
};

// src1 ^ src2
template <typename Type, typename TSrc1, typename TSrc2>
struct BitXor : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>( TSrc1()(opState) );
        Type rhs = static_cast<Type>( TSrc2()(opState) );

        return lhs ^ rhs;
    }
};

// eqv
template <typename Type, typename TSrc1, typename TSrc2>
struct BitXorNot : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>( TSrc1()(opState) );
        Type rhs = static_cast<Type>( TSrc2()(opState) );

        return lhs ^ ~rhs;
    }
};

template <typename Type, typename TSrc1>
struct BitNot : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = static_cast<Type>( TSrc1()(opState) );

        return ~lhs;
    }
};

template <typename Type, typename TSrc, typename TBegin, typename TCount>
struct BitExtract : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type value = static_cast<Type>( TSrc()(opState) );

        return (value >> (size_t)TBegin()(opState)) & shttl::mask(0, (size_t)TCount()(opState));
    }
};

// **********************************
//    counting zero operations
// **********************************
template <typename Type, typename TSrc>
struct NumberOfPopulations : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        u64 value = static_cast<u64>( TSrc()(opState));

        value = (value & 0x55555555) + (value >> 1 & 0x55555555);
        value = (value & 0x33333333) + (value >> 2 & 0x33333333);
        value = (value & 0x0f0f0f0f) + (value >> 4 & 0x0f0f0f0f);
        value = (value & 0x00ff00ff) + (value >> 8 & 0x00ff00ff);
        return static_cast<Type>((value & 0x0000ffff) + (value >>16 & 0x0000ffff));
    }
};

template <typename Type, typename TSrc>
struct NumberOfLeadingZeros : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        u64 value = static_cast<u64>( TSrc()(opState));

        value |= (value >> 1);
        value |= (value >> 2);
        value |= (value >> 4);
        value |= (value >> 8);
        value |= (value >> 16);

        value = ~value;

        value = (value & 0x55555555) + (value >> 1 & 0x55555555);
        value = (value & 0x33333333) + (value >> 2 & 0x33333333);
        value = (value & 0x0f0f0f0f) + (value >> 4 & 0x0f0f0f0f);
        value = (value & 0x00ff00ff) + (value >> 8 & 0x00ff00ff);
        return static_cast<Type>((value & 0x0000ffff) + (value >>16 & 0x0000ffff));
    }
};

template <typename Type, typename TSrc>
struct NumberOfTrailingZeros : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        s64 value = static_cast<Type>( TSrc()(opState));
        value = (value&(-value))-1;

        value = (value & 0x55555555) + (value >> 1 & 0x55555555);
        value = (value & 0x33333333) + (value >> 2 & 0x33333333);
        value = (value & 0x0f0f0f0f) + (value >> 4 & 0x0f0f0f0f);
        value = (value & 0x00ff00ff) + (value >> 8 & 0x00ff00ff);
        return static_cast<Type>((value & 0x0000ffff) + (value >>16 & 0x0000ffff));
    }
};


// **********************************
//    fp arithmetic operations
// **********************************

// sign from src1 and the rest from src2
template <typename TSrc1, typename TSrc2>
struct FPDoubleCopySign : public unary_function<OpEmulationState*, u64>
{
    u64 operator()(OpEmulationState* opState)
    {
        u64 lhs = static_cast<u64>( TSrc1()(opState) );
        u64 rhs = static_cast<u64>( TSrc2()(opState) );

        return (lhs & (u64)0x8000000000000000ULL) | (rhs & ~(u64)0x8000000000000000ULL);
    }
};

template <typename TSrc1, typename TSrc2>
struct FPDoubleCopySignNeg : public unary_function<OpEmulationState*, u64>
{
    u64 operator()(OpEmulationState* opState)
    {
        u64 lhs = static_cast<u64>( TSrc1()(opState) );
        u64 rhs = static_cast<u64>( TSrc2()(opState) );

        return (~lhs & (u64)0x8000000000000000ULL) | (rhs & ~(u64)0x8000000000000000ULL);
    }
};

// sign & exp from src1 and mantissa from src2
template <typename TSrc1, typename TSrc2>
struct FPDoubleCopySignExp : public unary_function<OpEmulationState*, u64>
{
    u64 operator()(OpEmulationState* opState)
    {
        u64 lhs = static_cast<u64>( TSrc1()(opState) );
        u64 rhs = static_cast<u64>( TSrc2()(opState) );

        return (lhs & (u64)0xfff0000000000000ULL) | (rhs & ~(u64)0xfff0000000000000ULL);
    }
};


template <typename Type, typename TSrc1>
struct FPNeg : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        return -TSrc1()(opState);
    }
};

template <typename Type, typename TSrc1>
struct FPAbs : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        return std::fabs( TSrc1()(opState) );
    }
};

template <typename Type, typename TSrc1, typename TSrc2, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
struct FPAdd : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = TSrc1()(opState);
        Type rhs = TSrc2()(opState);

        Onikiri::ScopedFESetRound sr( RoundMode()(opState) );
        volatile Type rvalue = lhs + rhs;
        return rvalue;
    }
};

template <typename Type, typename TSrc1, typename TSrc2, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
struct FPSub : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = TSrc1()(opState);
        Type rhs = TSrc2()(opState);

        Onikiri::ScopedFESetRound sr( RoundMode()(opState) );
        volatile Type rvalue = lhs - rhs;
        return rvalue;
    }
};

template <typename Type, typename TSrc1, typename TSrc2, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
struct FPMul : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = TSrc1()(opState);
        Type rhs = TSrc2()(opState);

        Onikiri::ScopedFESetRound sr( RoundMode()(opState) );
        volatile Type rvalue = lhs * rhs;
        return rvalue;
    }
};

template <typename Type, typename TSrc1, typename TSrc2, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
struct FPDiv : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Type lhs = TSrc1()(opState);
        Type rhs = TSrc2()(opState);

        Onikiri::ScopedFESetRound sr( RoundMode()(opState) );
        volatile Type rvalue = lhs / rhs;
        return rvalue;
    }
};

template <typename Type, typename TSrc1, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
struct FPSqrt : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        Onikiri::ScopedFESetRound sr( RoundMode()(opState) );
        volatile Type rvalue = std::sqrt( TSrc1()(opState) );
        return rvalue;
    }
};

template <typename Type, typename TSrc1, typename TSrc2, typename RoundMode = IntConst<int, FE_ROUNDDEFAULT> >
struct FPSubTest : public unary_function<OpEmulationState*, Type>
{
    Type operator()(OpEmulationState* opState)
    {
        volatile Type lhs = TSrc1()(opState);
        volatile Type rhs = TSrc2()(opState);

        Onikiri::ScopedFESetRound sr( RoundMode()(opState) );
        volatile Type rvalue = lhs - rhs;
        return rvalue;
    }
};

template < int lhs, u64 rhs, int mode, u64 correct >
static bool FpSubTestFunc( OpEmulationState* opState )
{
    typedef double                               Dst;
    typedef Cast< double, IntConst< int, lhs > > Src1;
    typedef AsFP< double, IntConst< u64, rhs > > Src2;
    typedef IntConst< int, mode >                Round;

    Dst result = FPSubTest< Dst, Src1, Src2, Round >()( opState );
    return AsIntFunc< u64, double >( result ) == correct;
}

inline void testroundmode(OpEmulationState* opState)
{
    bool testResult;

    // IEEE double has a significand with 53 bits of precision.
    // So, we test rounding modes with 1 (or -1) subtracted by 1.xx * 2^(-53).
    // The results are rounded to [-]1 - (1 * 2^(-53)) or [-]1 - (2 * 2^(-53))
    // depending on the rounding modes.

    // Double precision floating-point numbers are displayed in hexadecimal as follows:
    // 1.75 * 2^(-53) == 0x3cac000000000000
    // 1.25 * 2^(-53) == 0x3ca4000000000000
    // 1 - (1 * 2^(-53)) == 0x3fefffffffffffff
    // 1 - (2 * 2^(-53)) == 0x3feffffffffffffe
    // -1 - (1 * 2^(-53)) == 0xbff0000000000000
    // -1 - (2 * 2^(-53)) == 0xbff0000000000001

    testResult = FpSubTestFunc< 1,0x3cac000000000000,FE_DOWNWARD,0x3feffffffffffffe >(opState)
        && FpSubTestFunc< 1,0x3cac000000000000,FE_UPWARD,0x3fefffffffffffff >(opState)
        && FpSubTestFunc< 1,0x3cac000000000000,FE_TONEAREST,0x3feffffffffffffe >(opState)
        && FpSubTestFunc< 1,0x3cac000000000000,FE_TOWARDZERO,0x3feffffffffffffe >(opState)
        && FpSubTestFunc< 1,0x3ca4000000000000,FE_DOWNWARD,0x3feffffffffffffe >(opState)
        && FpSubTestFunc< 1,0x3ca4000000000000,FE_UPWARD,0x3fefffffffffffff >(opState)
        && FpSubTestFunc< 1,0x3ca4000000000000,FE_TONEAREST,0x3fefffffffffffff >(opState)
        && FpSubTestFunc< 1,0x3ca4000000000000,FE_TOWARDZERO,0x3feffffffffffffe >(opState)
        && FpSubTestFunc< -1,0x3cac000000000000,FE_DOWNWARD,0xbff0000000000001 >(opState)
        && FpSubTestFunc< -1,0x3cac000000000000,FE_UPWARD,0xbff0000000000000 >(opState)
        && FpSubTestFunc< -1,0x3cac000000000000,FE_TONEAREST,0xbff0000000000001 >(opState)
        && FpSubTestFunc< -1,0x3cac000000000000,FE_TOWARDZERO,0xbff0000000000000 >(opState);

    if (!testResult)
    {
        g_env.Print("\nFP rounding mode test failed. Programs may not be executed precisely.\n");
    }
}


// **********************************
//    branch operations
// **********************************

// relative: next_pc + disp*sizeof(Code)
// 
// call saves next_pc to TDest
template <typename TSrcDisp>
inline void BranchRelUncond(OpEmulationState* opState)
{
    u64 target = next_pc(opState) + 4*cast_to_signed( TSrcDisp()(opState) );
    do_branch(opState, target);

    opState->SetTakenPC(target);
}

template <typename TSrcTarget>
inline void BranchAbsUncond(OpEmulationState* opState)
{
    // <TODO> 0x03
    RegisterType addr = TSrcTarget()(opState) & ~(RegisterType)0x03;
    do_branch(opState, addr );

    opState->SetTakenPC(addr);
}

template <typename TSrcDisp, typename TCond>
inline void BranchRelCond(OpEmulationState* opState)
{
    if ( TCond()(opState) ) {
        BranchRelUncond<TSrcDisp>(opState);
    }
    else {
        opState->SetTakenPC( next_pc(opState) + 4*cast_to_signed( TSrcDisp()(opState) ));
    }
}

template <typename TSrcTarget, typename TCond>
inline void BranchAbsCond(OpEmulationState* opState)
{
    if ( TCond()(opState) ) {
        BranchAbsUncond<TSrcTarget>(opState);
    }
    else {
        RegisterType addr = TSrcTarget()(opState) & ~(RegisterType)0x03;
        opState->SetTakenPC( addr );
    }
}


template <typename TDest, typename TSrcDisp>
inline void CallRelUncond(OpEmulationState* opState)
{
    RegisterType ret_addr = static_cast<RegisterType>( next_pc(opState) );
    BranchRelUncond<TSrcDisp>(opState);
    TDest::SetOperand(opState, ret_addr );
}

template <typename TDest, typename TSrcTarget>
inline void CallAbsUncond(OpEmulationState* opState)
{
    // <TODO> 0x03
    RegisterType ret_addr = static_cast<RegisterType>( next_pc(opState) );
    BranchAbsUncond<TSrcTarget>(opState);
    TDest::SetOperand(opState, ret_addr );
}

} // namespace Operation {
} // namespace EmulatorUtility {
} // namespace Onikiri

#endif // #ifndef EMU_UTILITY_EMULATORUTILITY_GENERIC_OPERATION_H

