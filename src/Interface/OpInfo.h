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


#ifndef INTERFACE_OP_INFO_H
#define INTERFACE_OP_INFO_H

#include "Interface/Addr.h"
#include "Interface/OpClass.h"

namespace Onikiri 
{
    // Static information of an op.
    class OpInfo 
    {
    public:
        virtual ~OpInfo(){}

        // Op type
        virtual const OpClass& GetOpClass() const = 0;

        // Operands
        virtual int GetSrcOperand(const int index) const = 0;
        virtual int GetDstOperand(const int index) const = 0;

        // The number of operands.
        virtual int GetSrcNum() const = 0;
        virtual int GetDstNum() const = 0;

        // The number of micro-ops for its instruction.
        virtual int GetMicroOpNum() const = 0;

        // The position of this micro op in its instruction.
        virtual int GetMicroOpIndex() const = 0;

        // Instruction word size in byte
        virtual int GetInstructionSizeInByte() const = 0;

        // Mnemonic for dump.
        virtual const char* GetMnemonic() const = 0;
    };

}; // namespace Onikiri

#endif // INTERFACE_OP_INFO_H

