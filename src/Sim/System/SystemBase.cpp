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
#include <cinttypes>
#include "Sim/System/SystemBase.h"

using namespace std;
using namespace boost;
using namespace Onikiri;

SystemBase::SystemContext::SystemContext()
{
    executionCycles = 0;    
    executionInsns = 0;
    executedCycles = 0; 
    
    globalClock = 0;
    emulator = 0;
    resBuilder = 0;
}

SystemBase::SystemBase()
{
    m_context = nullptr;
    m_systemManager = NULL;
}

SystemBase::~SystemBase()
{
}

void SystemBase::SetSystemManager( SystemManagerIF* systemManager )
{
    m_systemManager = systemManager;
}

void SystemBase::Run()
{
}

bool SystemBase::NotifySyscallInvoke(SyscallNotifyContextIF* context, int pid, int tid) {

    u64 syscallNum = context->GetArg(0);
    switch (syscallNum) {
        // Print string
        case ONIKIRI_SYSCALL_PRINT: {
            g_env.Print("ONIKIRI_SYSCALL_PRINT: ");
            String str;
            for (int i = 0; i < 256; i++) {
                // Get one byte from emulator
                MemAccess mem;
                mem.address.address = context->GetArg(1) + i;
                mem.address.pid = pid;
                mem.address.tid = tid;
                mem.size = 1;
                GetSystemContext()->emulator->GetMemImage()->Read(&mem);
                if (mem.value == 0) {
                    break;
                }
                str += (char)mem.value;
            }
            g_env.Print(str + "\n");
            break;
        }
        case ONIKIRI_SYSCALL_TERMINATE_CURRENT_SYSTEM: {
            g_env.Print("ONIKIRI_SYSCALL_TERMINATE_CURRENT_SYSTEM \n");
            Terminate();
            break;
        }
        case ONIKIRI_SYSCALL_PRINT_CURRENT_STATUS: {
            const auto* sysContext = GetSystemContext();
            for (int i = 0; i < sysContext->threads.GetSize(); ++i) {
                if (sysContext->threads[i]->GetTID() == tid) {
                    g_env.Print(
                        "ONIKIRI_SYSCALL_PRINT_CURRENT_STATUS: retired_insns=%" PRIu64 " (pid=%d, tid=%d), cycle=%" PRId64 "\n", 
                        sysContext->threads[i]->GetOpRetiredID() , 
                        pid, 
                        tid, 
                        sysContext->globalClock->GetTick()
                    );
                }
             }
             break;
        }
    }

    // This system call is processed in simulation land
    // It must return true for avoiding unknown syscall error.
    if (ONIKIRI_SYSCALL_NUM_BEGIN <= syscallNum && syscallNum <= ONIKIRI_SYSCALL_NUM_END) {
        return true;
    }

    return false;
};
