// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Masahiro Goshima.
// Copyright (c) 2005-2015 Ryota Shioya.
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
#include "Emu/RISCV64Linux/RISCV64Info.h"
#include "Emu/RISCV64Linux/RISCV64Converter.h"

using namespace std;
using namespace Onikiri;
using namespace RISCV64Linux;

ISA_TYPE RISCV64Info::GetISAType()
{
    return ISA_ALPHA;
}

int RISCV64Info::GetRegisterSegmentID(int regNum)
{
    const int segmentRange[] = {
         0,     // Int
        32,     // FP
        64,     // Address
		4161,     // CSR
        4162      // 最大+1
    };
    const int nElems = sizeof(segmentRange)/sizeof(segmentRange[0]);

    ASSERT(0 <= regNum && regNum < RegisterCount, "regNum out of bound");
    ASSERT(segmentRange[nElems-1] == RegisterCount);

    return static_cast<int>( upper_bound(segmentRange, segmentRange+nElems, regNum) - segmentRange ) - 1;
}

int RISCV64Info::GetRegisterSegmentCount()
{
    return 4; // INT/FP/Address(for micro op)/FPCR
}

int RISCV64Info::GetInstructionWordBitSize()
{
    return InstructionWordBitSize;
}

int RISCV64Info::GetRegisterWordBitSize()
{
    return 64;
}

int RISCV64Info::GetRegisterCount()
{
    return RegisterCount;
}

int RISCV64Info::GetAddressSpaceBitSize()
{
    return 64;
}

int RISCV64Info::GetMaxSrcRegCount()
{
    return MaxSrcRegCount;
}

int RISCV64Info::GetMaxDstRegCount()
{
    return MaxDstRegCount;
}

int RISCV64Info::GetMaxOpInfoCountPerPC()
{
    return RISCV64ConverterTraits::MaxOpInfoDefs;
}

int RISCV64Info::GetMaxMemoryAccessByteSize()
{
    return MAX_MEMORY_ACCESS_WIDTH;
}

bool RISCV64Info::IsLittleEndian()
{
    return true;
}
