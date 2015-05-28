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


#ifndef INTERFACE_MEM_ACCESS_H
#define INTERFACE_MEM_ACCESS_H

#include <string>
#include "Types.h"
#include "Interface/Addr.h"

namespace Onikiri 
{

    // A structure for memory accesses interface
    // between a emulation part and a simulation part.
    struct MemAccess
    {

        // Codes of a memory access result.
        // These codes need to be defined in a interface section.
        // This is because a emulator notifies that memory accesses are 
        // invalid to a simulator on speculative memory accesses.
        enum Result
        {
            MAR_SUCCESS,
            MAR_READ_INVALID_ADDRESS,
            MAR_READ_NOT_READABLE,
            MAR_READ_UNALIGNED_ADDRESS,
            MAR_READ_INVALID_PARTIAL_READ,
            MAR_WRITE_INVALID_ADDRESS,
            MAR_WRITE_UNALIGNED_ADDRESS,
            MAR_WRITE_NOT_WRITABLE
        };

        Addr   address;
        int    size;
        bool   sign;
        u64    value;
        Result result;

        MemAccess() : size(0), sign(false), value(0), result(MAR_SUCCESS)
        {}

        const std::string ToString() const;
    };

}; // namespace Onikiri


#endif // INTERFACE_MEM_ACCESS_H

