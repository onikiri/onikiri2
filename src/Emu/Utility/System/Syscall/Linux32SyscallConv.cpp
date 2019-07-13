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
#include <time.h>

#include "Emu/Utility/System/Syscall/Linux32SyscallConv.h"

#include "Interface/SystemIF.h"
#include "Env/Env.h"
#include "SysDeps/posix.h"
#include "SysDeps/Endian.h"

#include "Emu/Utility/System/ProcessState.h"
#include "Emu/Utility/System/Memory/MemorySystem.h"
#include "Emu/Utility/System/Memory/MemoryUtility.h"
#include "Emu/Utility/System/VirtualSystem.h"
#include "Emu/Utility/System/Syscall/SyscallConstConv.h"
#include "Emu/Utility/OpEmulationState.h"

//#define SYSCALL_DEBUG

using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::POSIX;

namespace {
    const int LINUX_AT_FDCWD = -100;
}

// Linux32SyscallConv
Linux32SyscallConv::Linux32SyscallConv(ProcessState* processState)
    : Linux64SyscallConv(processState)
{
}

Linux32SyscallConv::~Linux32SyscallConv()
{
}

void Linux32SyscallConv::syscall_openat(OpEmulationState* opState)
{
    s32 fd = (s32)m_args[1];
    std::string fileName = StrCpyToHost(GetMemorySystem(), m_args[2]);
    int result = -1;

    if (fileName == std::string("/dev/tty")) {
        result = 1;
    }
    else {
        if (fd == LINUX_AT_FDCWD) {
            result = GetVirtualSystem()->Open(
                fileName.c_str(),
                (int)OpenFlagTargetToHost(static_cast<u32>(m_args[3]))
            );
        }
        else {
            THROW_RUNTIME_ERROR(
                "'openat' does not support reading fd other than 'AT_FDCWD (-100)', "
                "but '%d' is specified.", fd
            );
        }

    }

    if (result == -1)
        SetResult(false, GetVirtualSystem()->GetErrno());
    else
        SetResult(true, result);
}

void Linux32SyscallConv::syscall_faccessat(OpEmulationState* opState)
{
    s32 fd = m_args[1];
    //s32 flags = m_args[4];
    string path = StrCpyToHost(GetMemorySystem(), m_args[2]);
    int result = -1;
    /*
    ファイルディスクリプタがAT_FDCWD (-100)の場合はworking directoryからの相対パスとなる
    なので通常のaccessと同じ動作をする
    */
    if (fd == LINUX_AT_FDCWD) {
        result = GetVirtualSystem()->Access(path.c_str(), (int)AccessModeTargetToHost((u32)m_args[3]));
    }
    else {
        THROW_RUNTIME_ERROR(
            "'faccessat' does not support reading fd other than 'AT_FDCWD (-100)', "
            "but '%d' is specified.", fd
        );
    }

    if (result == -1)
        SetResult(false, GetVirtualSystem()->GetErrno());
    else
        SetResult(true, result);
}


