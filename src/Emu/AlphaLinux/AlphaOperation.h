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


#ifndef EMU_ALPHA_LINUX_ALPHALINUX_ALPHAOPERATION_H
#define EMU_ALPHA_LINUX_ALPHALINUX_ALPHAOPERATION_H

#include "SysDeps/fenv.h"
#include "Utility/RuntimeError.h"
#include "Emu/Utility/GenericOperation.h"
#include "Emu/Utility/System/Syscall/SyscallConvIF.h"
#include "Emu/Utility/System/ProcessState.h"


namespace Onikiri {
namespace AlphaLinux {
namespace Operation {

using namespace EmulatorUtility;
using namespace EmulatorUtility::Operation;

const RegisterType REG_VALUE_TRUE = 1;
const RegisterType REG_VALUE_FALSE = 0;

inline void AlphaPALHalt(EmulatorUtility::OpEmulationState* opState)
{
    THROW_RUNTIME_ERROR("AlphaPALHalt called.");    // <TODO>
}

// memory barrier
inline void AlphaPALIMB(EmulatorUtility::OpEmulationState* opState)
{
    // シングルスレッドなので何もしない
}

// read thread-unique value
inline void AlphaPALRdUniq(EmulatorUtility::OpEmulationState* opState)
{
    DstOperand<0>::SetOperand( opState, opState->GetProcessState()->GetThreadUniqueValue() );
}

// write thread-unique value
inline void AlphaPALWrUniq(EmulatorUtility::OpEmulationState* opState)
{
    opState->GetProcessState()->SetThreadUniqueValue( SrcOperand<0>()(opState) );
}

inline void AlphaPALGenTrap(EmulatorUtility::OpEmulationState* opState)
{
    THROW_RUNTIME_ERROR("AlphaPALGenTrap called."); // <TODO>
}


//
// system call
//
// v0 = $0 
// a0 = $16
//
// input:
// v0: syscall number
// a0-a4: arg1-arg5
//
// output:
// a3 : error flag
// v0 : return value or errno (if error)

inline void AlphaSyscallSetArg(EmulatorUtility::OpEmulationState* opState)
{
    EmulatorUtility::SyscallConvIF* syscallConv = opState->GetProcessState()->GetSyscallConv();
    syscallConv->SetArg(0, SrcOperand<0>()(opState));
    syscallConv->SetArg(1, SrcOperand<1>()(opState));
    syscallConv->SetArg(2, SrcOperand<2>()(opState));
}

// invoke syscall, get result&error and branch if any
inline void AlphaSyscallCore(EmulatorUtility::OpEmulationState* opState)
{
    EmulatorUtility::SyscallConvIF* syscallConv = opState->GetProcessState()->GetSyscallConv();
    syscallConv->SetArg(3, SrcOperand<0>()(opState));
    syscallConv->SetArg(4, SrcOperand<1>()(opState));
    syscallConv->SetArg(5, SrcOperand<2>()(opState));
    syscallConv->Execute(opState);

    DstOperand<0>::SetOperand(opState, syscallConv->GetResult(EmulatorUtility::SyscallConvIF::RetValueIndex) );
    DstOperand<1>::SetOperand(opState, syscallConv->GetResult(EmulatorUtility::SyscallConvIF::ErrorFlagIndex) );
}


// **********************************
//              compare
// **********************************
template <typename TSrc1, typename TSrc2, typename Comp>
struct AlphaCompare : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        if ( Comp()(TSrc1()(opState), TSrc2()(opState)) ) {
            return (u64)REG_VALUE_TRUE;
        }
        else {
            return (u64)REG_VALUE_FALSE;
        }
    }
};

template <typename TSrc1, typename TSrc2, typename Comp>
struct AlphaTFloatCompare : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        if ( Comp()(TSrc1()(opState), TSrc2()(opState)) ) {
            return 0x4000000000000000LL;
        }
        else {
            return 0x0000000000000000LL;
        }
    }
};

// cmptun
template<typename Type, typename ArgType = RegisterType>
struct AlphaFPCondUnordered : public std::binary_function<ArgType, ArgType, bool>
{
    bool operator()(const ArgType lhs, const ArgType rhs) const
    {
        return IsNAN(AsFPFunc<Type>(lhs)) || IsNAN(AsFPFunc<Type>(rhs));
    }
};

template<typename TSrc1, typename TSrc2>
struct AlphaAddr : public std::unary_function<EmulatorUtility::OpEmulationState, RegisterType>
{
    RegisterType operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        return TSrc1()(opState) + EmulatorUtility::cast_to_signed( TSrc2()(opState) );
    }
};

// ldq_u, stq_u 用アドレス計算
template<typename TSrc1, typename TSrc2>
struct AlphaAddrUnaligned : public std::unary_function<EmulatorUtility::OpEmulationState, RegisterType>
{
    RegisterType operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        return ( TSrc1()(opState) + EmulatorUtility::cast_to_signed( TSrc2()(opState) ) ) & ~(RegisterType)0x07;
    }
};

template<typename TSrc1, typename TSrc2>
struct AlphaLdah : public std::unary_function<EmulatorUtility::OpEmulationState, RegisterType>
{
    RegisterType operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        return TSrc1()(opState) + EmulatorUtility::cast_to_signed( TSrc2()(opState) << 16);
    }
};

// **********************************
//    byte-wise operations
// **********************************

// byte-wise compare
template<typename TSrc1, typename TSrc2>
struct AlphaCmpBge : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        u64 result = 0;
        u64 lhs = TSrc1()(opState);
        u64 rhs = TSrc2()(opState);

        for (int i = 0; i < 8; i ++) {
            if ((lhs & 0xff) >= (rhs & 0xff)) {
                result |= 1 << 8;
            }

            lhs >>= 8;
            rhs >>= 8;
            result >>= 1;
        }

        return result;  
    }
};

template <typename Type, typename TSrc1, typename TSrc2>
struct AlphaExtxl : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        u64 result = 0;
        u64 lhs = TSrc1()(opState);
        u64 rhs = TSrc2()(opState);

        int shift_cnt = static_cast<int>( (rhs & 0x7) << 3 );
        int byte_loc = shift_cnt;
        result = lhs >> byte_loc;

        return static_cast<Type>(result);
    }
};

template <typename Type, typename TSrc1, typename TSrc2>
struct AlphaExtxh : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        u64 result = 0;
        u64 lhs = TSrc1()(opState);
        u64 rhs = TSrc2()(opState);

        int shift_cnt = static_cast<int>( (rhs & 0x7) << 3 );
        int byte_loc = 64 - shift_cnt;
        result = lhs << (byte_loc & 0x3f);
        
        // To avoid a optimization bug in VS2010 SP1.
        // Left shift is mistakenly done on u16 value...
        if( (size_t)( byte_loc & 0x3f ) >= sizeof(Type)*8 ){
            result = 0;
        }

        return  static_cast<Type>(result);
    }
};

template <typename Type, typename TSrc1, typename TSrc2>
struct AlphaInsxl : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        u64 lhs = TSrc1()(opState);
        u64 rhs = TSrc2()(opState);

        int shift_cnt = static_cast<int>( (rhs & 0x7) << 3 );
        int byte_loc = shift_cnt;

        u64 result = static_cast<Type>(lhs);
        result <<= byte_loc;

        return result;
    }
};

template <typename Type, typename TSrc1, typename TSrc2>
struct AlphaInsxh : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        u64 lhs = TSrc1()(opState);
        u64 rhs = TSrc2()(opState);

        int shift_cnt = static_cast<int>( (rhs & 0x7) << 3 );
        int byte_loc = 64 - shift_cnt;
        u64 bit_mask;

        if (shift_cnt + sizeof(Type)*8 > 64 ) {
            bit_mask = ~(~(u64)0 << (shift_cnt + sizeof(Type)*8 - 64));
        }
        else {
            bit_mask = 0;
        }

        u64 result;
        result = lhs >> (byte_loc & 0x3f);
        result &= bit_mask;

        return result;
    }
};

template <typename Type, typename TSrc1, typename TSrc2>
struct AlphaMskxl : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        u64 lhs = TSrc1()(opState);
        u64 rhs = TSrc2()(opState);

        int shift_cnt = static_cast<int>( (rhs & 0x7) << 3 );
        u64 bit_mask;
        int type_bits = sizeof(Type) * 8;
        if (type_bits < 64)
            bit_mask = (((u64)1 << type_bits) - 1);
        else
            bit_mask = ~((u64)0);
        bit_mask <<= shift_cnt;
        bit_mask = ~bit_mask;

        u64 result;
        result = lhs & bit_mask;

        return result;
    }
};

template <typename Type, typename TSrc1, typename TSrc2>
struct AlphaMskxh : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        u64 lhs = TSrc1()(opState);
        u64 rhs = TSrc2()(opState);

        int shift_cnt = static_cast<int>( (rhs & 0x7) << 3 );
        u64 bit_mask;

        if (shift_cnt + sizeof(Type)*8 > 64 ) {
            bit_mask = ~(u64)0 << (shift_cnt + sizeof(Type)*8 - 64);
        }
        else {
            bit_mask = ~(u64)0;
        }

        u64 result;
        result = lhs & bit_mask;

        return result;
    }
};

inline u64 AlphaZapNotBitMask(u64 byte_mask)
{
    u64 result = 0;
    
    for (int i = 0; i < 8; i ++) {
        result <<= 8;
        result |= (((byte_mask & 0x80) >> 7) - 1) & 0xff;
        byte_mask <<= 1;
    }

    return result;
}

template <typename TSrc1, typename TSrc2>
struct AlphaZap : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        u64 lhs = TSrc1()(opState);
        u64 rhs = TSrc2()(opState);
        
        return lhs & AlphaZapNotBitMask(rhs);
    }
};

template <typename TSrc1, typename TSrc2>
struct AlphaZapNot : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        u64 lhs = TSrc1()(opState);
        u64 rhs = TSrc2()(opState);
        
        return lhs & ~AlphaZapNotBitMask(rhs);
    }
};

// BWX
template <typename TSrc1>
struct AlphaSextb : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        s64 val = EmulatorUtility::cast_to_signed((u8)TSrc1()(opState));
        return val;
    }
};

template <typename TSrc1>
struct AlphaSextw : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        s64 val = EmulatorUtility::cast_to_signed((u16)TSrc1()(opState));
        return val;
    }
};

// FIX
template <typename TSrc1>
struct AlphaFtois : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        u64 va = TSrc1()(opState); 
        return (s32) (((va >> 32) & 0xc0000000) |
            ((va >> 29) & 0x3fffffff));
    }
};

template <typename TSrc1>
struct AlphaItofs : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        u64 result;
        u64 va = TSrc1()(opState);

        result = ((va & 0x7fffffff) << 29) | ((va & 0x80000000) << 32);
        if ((va & 0x7f800000) == 0x7f800000) {       // exp = ~0
            result |= (u64)0x7ff << 52;
        }
        else if ((va & 0x7f800000) == 0x00000000) {  // exp = 0
            // do nothing
        }
        else {
            result += (u64)(1023-127) << 52;            // exp' = exp+896
        }

        return result;
    }
};

// MAX
template <typename TSrc>
struct AlphaPackLongWords : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        u64 val = TSrc()(opState);

        return (val & 0xff) | (((val >> 32) & 0xff) << 8);
    }
};

template <typename TSrc>
struct AlphaPackWords : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        u64 val = TSrc()(opState);

        return (val & 0xff) | (((val >> 16) & 0xff) << 8) |
            (((val >> 32) & 0xff) << 16) | (((val >> 48) & 0xff) << 24);
    }
};

template <typename TSrc>
struct AlphaUnpackLongWords : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        u64 val = TSrc()(opState);

        return (val & 0xff) | (((val >> 8) & 0xff) << 32);
    }
};

template <typename TSrc>
struct AlphaUnpackWords : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        u64 val = TSrc()(opState);

        return (val & 0xff) | (((val >> 8) & 0xff) << 16) |
            (((val >> 16) & 0xff) << 32) | (((val >> 24) & 0xff) << 48);
    }
};

template <typename Type, typename TSrc1, typename TSrc2>
struct AlphaVectorMin : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        u64 src1 = TSrc1()(opState);
        u64 src2 = TSrc2()(opState);
        u64 retval = 0;
        int vsize = sizeof(Type);
        
        for(int i = 0; i < 8/vsize; i++)
        {
            Type tempSrc1 = (Type)((src1>>(8*i*vsize)) & (0xff<<((vsize-1)*8) | 0xff));
            Type tempSrc2 = (Type)((src2>>(8*i*vsize)) & (0xff<<((vsize-1)*8) | 0xff));
            retval |= std::min<Type>(tempSrc1,tempSrc2) << 8*i*vsize;
        }
        return retval;
    }
};

template <typename Type, typename TSrc1, typename TSrc2>
struct AlphaVectorMax : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        u64 src1 = TSrc1()(opState);
        u64 src2 = TSrc2()(opState);
        u64 retval = 0;
        int vsize = sizeof(Type);

        for(int i = 0; i < 8/vsize; i++)
        {
            Type tempSrc1 = (Type)((src1>>(8*i*vsize)) & (0xff<<((vsize-1)*8) | 0xff));
            Type tempSrc2 = (Type)((src2>>(8*i*vsize)) & (0xff<<((vsize-1)*8) | 0xff));
            retval |= std::max<Type>(tempSrc1,tempSrc2) << 8*i*vsize;
        }
        return retval;
    }
};

template <typename TSrc1, typename TSrc2>
struct AlphaPixelError : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        u64 src1 = TSrc1()(opState);
        u64 src2 = TSrc2()(opState);
        u64 retval = 0;
        
        for(int i = 0; i < 8; i++)
        {
            u64 tempSrc1 = src1>>(8*i) & 0xff;
            u64 tempSrc2 = src2>>(8*i) & 0xff;
            retval += (tempSrc1 > tempSrc2) ? tempSrc1 - tempSrc2 :
                                                tempSrc2 - tempSrc1;
        }
        return retval;
    }
};

// **********************************
//    s-float load/store
// **********************************

template <typename TSrcFPCR>
struct AlphaRoundModeFromFPCR : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    int operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        u64 fpcr = TSrcFPCR()(opState);

        switch ((fpcr >> 58) & 0x3) {   // FPCR の58:59ビットが丸めモード
        case 0x0:
            return FE_TOWARDZERO;
        case 0x1:
            return FE_DOWNWARD;
        case 0x2:
            return FE_TONEAREST;
        case 0x3:
            return FE_UPWARD;
        default:
            assert(0);  // never reached
            return FE_TOWARDZERO;
        }
    }
};

template <typename TSrc1>
struct AlphaCvtLQ : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        u64 va = TSrc1()(opState);
        u64 result;
        
        result = ((va >> 32) & 0xc0000000) |
                 ((va >> 29) & 0x3fffffff);
        return (u64)(s64)(s32)result;   // sign extend
    }
};

template <typename TSrc1>
struct AlphaCvtQL : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        u64 va = TSrc1()(opState);
        u64 result;
        
        result = ((va & 0xc0000000) << 32) |
                  ((va & 0x3fffffff) << 29);
        return result;
    }
};

template <typename TSrc1>
struct AlphaCvtQS : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        s64 va = TSrc1()(opState);

        return AsIntFunc<u64>( (double)(float)(va) );
    }
};

template <typename TSrc1>
struct AlphaCvtQT : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        s64 va = EmulatorUtility::cast_to_signed( TSrc1()(opState) );

        return AsIntFunc<u64>( (double)va );
    }
};

template <typename TSrc1>
struct AlphaCvtTS : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        double va = AsFPFunc<double>( TSrc1()(opState) );

        return AsIntFunc<u64>( (double)(float)va );
    }
};

template <typename TSrc1>
struct AlphaCvtTQ_c : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        double va = AsFPFunc<double>( TSrc1()(opState) );

        return (u64)(s64)va;
    }
};

template <typename TSrc1>
struct AlphaCvtTQ_m : public std::unary_function<EmulatorUtility::OpEmulationState, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        double va = AsFPFunc<double>( TSrc1()(opState) );

        return (u64)(s64)floor(va);
    }
};

// IEEE float -> IEEE double or load u32
template <typename TAddr>
struct AlphaLds : public std::unary_function<EmulatorUtility::OpEmulationState*, u64>
{
    u64 operator()(EmulatorUtility::OpEmulationState* opState)
    {
        u64 result;
        u64 va = ReadMemory<u32>( opState, TAddr()(opState) );

        result = ((va & 0x7fffffff) << 29) | ((va & 0x80000000) << 32);
        if ((va & 0x7f800000) == 0x7f800000) {       // exp = ~0
            result |= (u64)0x7ff << 52;
        }
        else if ((va & 0x7f800000) == 0x00000000) {  // exp = 0
            // do nothing
        }
        else {
            result += (u64)(1023-127) << 52;            // exp' = exp+896
        }

        return result;
    }
};

// IEEE double -> IEEE float or store u32
template <typename TSrc, typename TAddr>
inline void AlphaSts(EmulatorUtility::OpEmulationState* opState)
{
    u64 va = TSrc()(opState);

    WriteMemory<u32>( opState, TAddr()(opState), 
        (u32) (((va >> 32) & 0xc0000000) |
        ((va >> 29) & 0x3fffffff)));
}

} // namespace Operation {
} // namespace AlphaLinux {
} // namespace Onikiri

#endif // #ifndef EMU_ALPHA_LINUX_ALPHALINUX_ALPHAOPERATION_H

