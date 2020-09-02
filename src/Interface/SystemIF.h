// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Masahiro Goshima.
// Copyright (c) 2005-2020 Ryota Shioya.
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


#ifndef INTERFACE_SYSTEM_IF_H
#define INTERFACE_SYSTEM_IF_H

#include "Types.h"
#include "Interface/Addr.h"

namespace Onikiri 
{
    // Arguments and return values for NotifySyscallInvoke
    class SyscallNotifyContextIF {
    public:
        virtual ~SyscallNotifyContextIF() {};
        
        // Get argument values passed from syscall
        virtual u64 GetArg(int index) const = 0;

        // Set syscall results
        virtual void SetResult(bool success, u64 result) = 0;
    };
    
    class SystemIF 
    {
    public:
        SystemIF() {}
        virtual ~SystemIF() {}

        virtual void NotifyProcessTermination(int pid) = 0;
        virtual void NotifySyscallReadFileToMemory(const Addr& addr, u64 size) = 0;
        virtual void NotifySyscallWriteFileFromMemory(const Addr& addr, u64 size) = 0;
        virtual void NotifyMemoryAllocation(const Addr& addr, u64 size, bool allocate) = 0;

        // If this function returns true, processing system call is skipped.
        virtual bool NotifySyscallInvoke(SyscallNotifyContextIF* context) = 0;
    }; // class SystemIF

}; // namespace Onikiri

#endif // INTERFACE_SYSTEM_IF_H

