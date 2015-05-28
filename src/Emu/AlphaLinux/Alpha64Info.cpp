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
#include "Emu/AlphaLinux/Alpha64Info.h"
#include "Emu/AlphaLinux/Alpha64Converter.h"

using namespace std;
using namespace Onikiri;
using namespace AlphaLinux;

ISA_TYPE Alpha64Info::GetISAType()
{
    return ISA_ALPHA;
}

int Alpha64Info::GetRegisterSegmentID(int regNum)
{
    const int segmentRange[] = {
         0,     // Int
        32,     // FP
        64,     // Address
        65,     // FPCR
        66      // Н≈Се+1
    };
    const int nElems = sizeof(segmentRange)/sizeof(segmentRange[0]);

    ASSERT(0 <= regNum && regNum < RegisterCount, "regNum out of bound");
    ASSERT(segmentRange[nElems-1] == RegisterCount);

    return static_cast<int>( upper_bound(segmentRange, segmentRange+nElems, regNum) - segmentRange ) - 1;
}

int Alpha64Info::GetRegisterSegmentCount()
{
    return 4; // INT/FP/Address(for micro op)/FPCR
}

int Alpha64Info::GetInstructionWordBitSize()
{
    return InstructionWordBitSize;
}

int Alpha64Info::GetRegisterWordBitSize()
{
    return 64;
}

int Alpha64Info::GetRegisterCount()
{
    return RegisterCount;
}

int Alpha64Info::GetAddressSpaceBitSize()
{
    return 64;
}

int Alpha64Info::GetMaxSrcRegCount()
{
    return MaxSrcRegCount;
}

int Alpha64Info::GetMaxDstRegCount()
{
    return MaxDstRegCount;
}

int Alpha64Info::GetMaxOpInfoCountPerPC()
{
    return Alpha64ConverterTraits::MaxOpInfoDefs;
}

int Alpha64Info::GetMaxMemoryAccessByteSize()
{
    return MAX_MEMORY_ACCESS_WIDTH;
}

bool Alpha64Info::IsLittleEndian()
{
    return true;
}
