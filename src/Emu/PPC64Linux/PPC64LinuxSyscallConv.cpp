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

#include "Emu/PPC64Linux/PPC64LinuxSyscallConv.h"

#include "Env/Env.h"
#include "SysDeps/posix.h"

#include "Emu/Utility/System/ProcessState.h"
#include "Emu/Utility/System/Memory/MemorySystem.h"
#include "Emu/Utility/System/Memory/MemoryUtility.h"
#include "Emu/Utility/System/VirtualSystem.h"
#include "Emu/Utility/System/Syscall/SyscallConstConv.h"
#include "Emu/Utility/OpEmulationState.h"

#include "Emu/PPC64Linux/PPC64LinuxSyscallID.h"

#define SYSCALL_DEBUG

using namespace std;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::PPC64Linux;
using namespace Onikiri::POSIX;


PPC64LinuxSyscallConv::PPC64LinuxSyscallConv(ProcessState* processState) : Linux64SyscallConv(processState)
{
}

PPC64LinuxSyscallConv::~PPC64LinuxSyscallConv()
{
}

#ifdef SYSCALL_DEBUG
#define SYSCALLNAME(name, argcnt, argtempl) {syscall_id_##name, #name, argcnt, argtempl}

namespace {

// x: int (hexadecimal)
// n: int
// p: pointer
// s: string

static struct {
    unsigned int syscallNo;
    const char* name;
    int argcnt;
    const char* argtempl;
} syscallTable[] = {
    SYSCALLNAME(exit, 0, ""),
    SYSCALLNAME(read, 3, "npn"),
    SYSCALLNAME(write, 3, "npn"),
    SYSCALLNAME(readv, 3, "npn"),
    SYSCALLNAME(writev, 3, "npn"),
    SYSCALLNAME(open, 3, "sxx"),
    SYSCALLNAME(close, 1, "n"),
    SYSCALLNAME(stat, 2, "sp"),
    SYSCALLNAME(lstat, 2, "sp"),
    SYSCALLNAME(fstat, 2, "np"),
    //SYSCALLNAME(stat64, 2, "sp"),
    //SYSCALLNAME(lstat64, 2, "sp"),
    //SYSCALLNAME(fstat64, 2, "np"),
    SYSCALLNAME(lseek, 3, "nxn"),
    SYSCALLNAME(truncate, 2, "px"),
    SYSCALLNAME(ftruncate, 2, "nx"),
    SYSCALLNAME(fcntl, 3, "nnp"),
    SYSCALLNAME(chdir, 1, "p"),
    SYSCALLNAME(fchdir, 1, "n"),
    SYSCALLNAME(getcwd, 2, "px"),
    SYSCALLNAME(mkdir, 2, "sx"),
    SYSCALLNAME(rmdir, 1, "s"),
    SYSCALLNAME(readlink, 3, "spx"),
    SYSCALLNAME(unlink, 1, "s"),
    SYSCALLNAME(link, 2, "ss"),
    SYSCALLNAME(rename, 2, "ss"),
    SYSCALLNAME(mmap, 6, "pxxxnx"),
    SYSCALLNAME(mremap, 4, "pxxx"),
    SYSCALLNAME(munmap, 2, "px"),
    SYSCALLNAME(mprotect, 3, "pxx"),
    SYSCALLNAME(chmod, 2, "sx"),
    SYSCALLNAME(ioctl, 3, "xxx"),
    SYSCALLNAME(time, 1, "p"),
    SYSCALLNAME(times, 1, "p"),
    SYSCALLNAME(gettimeofday, 2, "pp"),
    //SYSCALLNAME(settimeofday, 2, "pp"),

    SYSCALLNAME(uname, 1, "p"),
    SYSCALLNAME(brk, 1, "p"),

    SYSCALLNAME(access, 2, "sx"),

    SYSCALLNAME(gettid, 0, ""),
    SYSCALLNAME(setuid, 1, "x"),
    SYSCALLNAME(getuid, 0, ""),
    //SYSCALLNAME(seteuid, 1, "x"),
    SYSCALLNAME(setgid, 1, "x"),
    SYSCALLNAME(getgid, 0, ""),
    //SYSCALLNAME(setegid, 1, "x"),
    SYSCALLNAME(geteuid, 0, ""),
    SYSCALLNAME(getegid, 0, ""),
    //SYSCALLNAME(setreuid, 2),
    //SYSCALLNAME(setregid, 2),
    SYSCALLNAME(getrusage, 2, "xp"),
    SYSCALLNAME(getrlimit, 2, "np"),
    SYSCALLNAME(setrlimit, 2, "np"),
    SYSCALLNAME(exit_group, 0, ""),
    SYSCALLNAME(dup, 1, "n"),

    SYSCALLNAME(rt_sigaction, 3, "xxx"),
    SYSCALLNAME(rt_sigprocmask, 3, "xxx"),
    SYSCALLNAME(kill, 2, "xx"),
    SYSCALLNAME(tgkill, 3, "xxx"),
};
#undef SYSCALLNAME

} // namespace {
#endif // #ifdef SYSCALL_DEBUG


void PPC64LinuxSyscallConv::Execute(OpEmulationState* opState)
{
#ifdef SYSCALL_DEBUG 
    static std::ofstream log("syscall.log", std::ios_base::out | std::ios_base::trunc);

    log << dec << GetArg(0) << " ";
    for (size_t i = 0; i < sizeof(syscallTable)/sizeof(syscallTable[0]); i ++) {
        if (syscallTable[i].syscallNo == GetArg(0)) {
            log << syscallTable[i].name << "(";
            int argcnt = syscallTable[i].argcnt;
            const char *argtempl = syscallTable[i].argtempl;
            for (int j = 1; j <= argcnt; j ++) {
                switch (argtempl[j-1]) {
                case 's':
                    {
                        std::string s = StrCpyToHost( GetMemorySystem(), GetArg(j) );
                        log << "0x" << hex << GetArg(j) << dec;
                        log << "=\"" << s << "\"";
                    }
                    break;
                case 'n':
                    log << dec << (s64)GetArg(j);
                    break;
                case 'p':
                case 'x':
                default:
                    log << "0x" << hex << GetArg(j) << dec;
                    break;
                }
                if (j != argcnt)
                    log << ", ";
            }
            log << ")";
        }
    }
    log << flush;
#endif // #ifdef SYSCALL_DEBUG

    opState->SetTakenPC( opState->GetPC()+4 );
    opState->SetTaken(true);

    SetResult(false, 0);
    switch (GetArg(0)) {
    case syscall_id_exit:
    case syscall_id_exit_group:
        syscall_exit(opState);
        break;
    case syscall_id_read:
        syscall_read(opState);
        break;
    case syscall_id_write:
        syscall_write(opState);
        break;
    case syscall_id_readv:
        syscall_readv(opState);
        break;
    case syscall_id_writev:
        syscall_writev(opState);
        break;
    case syscall_id_open:
        syscall_open(opState);
        break;
    case syscall_id_close:
        syscall_close(opState);
        break;

    case syscall_id_lseek:
        syscall_lseek(opState);
        break;
    case syscall_id_unlink:
        syscall_unlink(opState);
        break;
    case syscall_id_rename:
        syscall_rename(opState);
        break;


    case syscall_id_mmap:
        syscall_mmap(opState);
        break;
    case syscall_id_mremap:
        syscall_mremap(opState);
        break;
    case syscall_id_munmap:
        syscall_munmap(opState);
        break;
    //case syscall_id_mprotect:
    //  syscall_mprotect(opState);
    //  break;
    case syscall_id_uname:
        syscall_uname(opState);
        break;
    case syscall_id_fcntl:
        syscall_fcntl(opState);
        break;
    case syscall_id_brk:
        syscall_brk(opState);
        break;
    case syscall_id_getpid:
    case syscall_id_gettid:
        syscall_getpid(opState);
        break;
    ////case syscall_id_setuid:
    ////    syscall_setuid(opState);
    ////    break;
    case syscall_id_getuid:
        syscall_getuid(opState);
        break;
    case syscall_id_geteuid:
        syscall_geteuid(opState);
        break;
    case syscall_id_getgid:
        syscall_getgid(opState);
        break;
    case syscall_id_getegid:
        syscall_getegid(opState);
        break;
    //case syscall_id_setgid:
    //  syscall_setgid(opState);
    //  break;
    //case syscall_id_seteuid:
    //  syscall_seteuid(opState);
    //  break;
    //case syscall_id_setegid:
    //  syscall_setegid(opState);
    //  break;
    //case syscall_id_setregid:
    //  syscall_setregid(opState);
    //  break;
    //case syscall_id_setreuid:
    //  syscall_setreuid(opState);
    //  break;
    //case syscall_id_access:
    //  syscall_access(opState);
    //  break;
    case syscall_id_stat:
//  case syscall_id_stat64:
        syscall_stat64(opState);
        break;
    case syscall_id_lstat:
//  case syscall_id_lstat64:
        syscall_lstat64(opState);
        break;
    case syscall_id_fstat:
//  case syscall_id_fstat64:
        syscall_fstat64(opState);
        break;
    case syscall_id_ioctl:
        syscall_ioctl(opState);
        break;
    //case syscall_id_readlink:
    //  syscall_readlink(opState);
    //  break;
    case syscall_id_time:
        syscall_time(opState);
        break;
    case syscall_id_times:
        syscall_times(opState);
        break;
    case syscall_id_gettimeofday:
        syscall_gettimeofday(opState);
        break;

    case syscall_id_dup:
        syscall_dup(opState);
        break;
    case syscall_id_truncate:
        syscall_truncate(opState);
        break;
    case syscall_id_ftruncate:
        syscall_ftruncate(opState);
        break;

    case syscall_id_kill:
        syscall_kill(opState);
        break;
    case syscall_id_tgkill:
        syscall_tgkill(opState);
        break;


    // signal functions
    case syscall_id_rt_sigaction:
    case syscall_id_rt_sigprocmask:
        syscall_ignore(opState);
        break;

    // gcc が stack の拡張に使用するだけ
    case syscall_id_getrlimit:
        syscall_ignore(opState);
        break;
    case syscall_id_setrlimit:
        syscall_ignore(opState);
        break;

    //case syscall_id_mkdir:
    //  syscall_mkdir(opState);
    //  break;

    // gcc が実行時間の取得に使用
    case syscall_id_getrusage:
        syscall_ignore(opState);
        break;

    case syscall_id_getcwd:
        syscall_getcwd(opState);
        break;
    default:
        {
            stringstream ss;
            ss << "unknown syscall " << GetArg(0) << " called";
            THROW_RUNTIME_ERROR(ss.str().c_str());
        }
    }
#ifdef SYSCALL_DEBUG
    if (GetResult(ErrorFlagIndex))
        log << " = error " << hex << GetResult(RetValueIndex) << std::endl;
    else
        log << " = " << hex << GetResult(RetValueIndex) << std::endl;
#endif // #ifdef DEBUG
}

namespace {
    const int PPC64_CLK_TCK = 100;

    const int PPC64_MREMAP_MAYMOVE = 1;
    const int PPC64_MAP_NORESERVE = 0x00040;
    const int PPC64_MAP_ANONYMOUS = 0x20;
}

int PPC64LinuxSyscallConv::Get_MAP_ANONYMOUS()
{
    return PPC64_MAP_ANONYMOUS;
}

int PPC64LinuxSyscallConv::Get_MREMAP_MAYMOVE()
{
    return PPC64_MREMAP_MAYMOVE;
}

int PPC64LinuxSyscallConv::Get_CLK_TCK()
{
    return PPC64_CLK_TCK;
}

u32 PPC64LinuxSyscallConv::OpenFlagTargetToHost(u32 flag)
{
    static u32 host_open_flags[] =
    {
        POSIX_O_RDONLY, POSIX_O_WRONLY, POSIX_O_RDWR,
        POSIX_O_APPEND,
        POSIX_O_CREAT,
        POSIX_O_EXCL,
        //O_NOCTTY,
        //O_NONBLOCK,
        //O_SYNC,
        POSIX_O_TRUNC,
    };
    // see bits/fcntl.h
    static u32 ppc64_open_flags[] =
    {
        00, 01, 02, // O_RDONLY, O_WRONLY, O_RDWR,
        02000,  // O_APPEND
        00100,  // O_CREAT
        00200,  // O_EXCL
        //00400,    // O_NOCTTY
        //04000,    // O_NONBLOCK
        //010000,   // O_SYNC
        01000,  // O_TRUNC
    };
    static int open_flags_size = sizeof(host_open_flags)/sizeof(host_open_flags[0]);

    SyscallConstConvBitwise conv(
        host_open_flags,
        ppc64_open_flags, 
        open_flags_size);

    return conv.TargetToHost(flag);
}
