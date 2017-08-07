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
#include "Utility/RuntimeError.h"
#include "Emu/Utility/GenericOperation.h"
#include "Emu/Utility/System/Syscall/SyscallConvIF.h"
#include "Emu/Utility/System/ProcessState.h"


namespace Onikiri {
namespace RISCV32Linux {
namespace Operation {

using namespace EmulatorUtility;
using namespace EmulatorUtility::Operation;

typedef u32 RISCV32RegisterType;

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
struct RISCV32SrcOperand : public std::unary_function<OpEmulationState, RISCV32RegisterType>
{
    typedef RISCV32RegisterType type;
    RISCV32RegisterType operator()(OpEmulationState* opState)
    {
        return (RISCV32RegisterType)opState->GetSrc(OperandIndex);
    }
};

template <typename Type, Type value>
struct RISCV32IntConst : public std::unary_function<OpEmulationState*, Type>
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
}

inline u32 RISCV32NextPC(OpEmulationState* opState)
{
    return (u32)opState->GetPC() + 4;
}

inline u32 RISCV32CurPC(OpEmulationState* opState)
{
    return (u32)opState->GetPC();
}


template<typename TSrc1>
struct RISCV32Auipc : public std::unary_function<OpEmulationState, RegisterType>
{
    RISCV32RegisterType operator()(OpEmulationState* opState) const
    {
        return (TSrc1()(opState) << 12) + RISCV32CurPC(opState);
    }
};


} // namespace Operation {
} // namespace RISCV32Linux {
} // namespace Onikiri

#endif // #ifndef EMU_ALPHA_LINUX_ALPHALINUX_ALPHAOPERATION_H

