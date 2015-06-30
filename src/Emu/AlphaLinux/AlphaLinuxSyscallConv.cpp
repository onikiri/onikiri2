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

#include "Emu/AlphaLinux/AlphaLinuxSyscallConv.h"

#include "Env/Env.h"
#include "SysDeps/posix.h"

#include "Emu/Utility/System/ProcessState.h"
#include "Emu/Utility/System/Memory/MemorySystem.h"
#include "Emu/Utility/System/Memory/MemoryUtility.h"
#include "Emu/Utility/System/VirtualSystem.h"
#include "Emu/Utility/System/Syscall/SyscallConstConv.h"
#include "Emu/Utility/OpEmulationState.h"

#include "Emu/AlphaLinux/AlphaLinuxSyscallID.h"


//#define SYSCALL_DEBUG

using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::AlphaLinux;
using namespace Onikiri::POSIX;



// AlphaLinuxSyscallConv
AlphaLinuxSyscallConv::AlphaLinuxSyscallConv(ProcessState* processState) : Linux64SyscallConv(processState)
{
}

AlphaLinuxSyscallConv::~AlphaLinuxSyscallConv()
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
    const char *const name;
    int argcnt;
    const char *const argtempl;
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
    SYSCALLNAME(stat64, 2, "sp"),
    SYSCALLNAME(lstat64, 2, "sp"),
    SYSCALLNAME(fstat64, 2, "np"),
    SYSCALLNAME(lseek, 3, "nxn"),
    SYSCALLNAME(truncate, 2, "px"),
    SYSCALLNAME(ftruncate, 2, "nx"),
    SYSCALLNAME(fcntl, 3, "nnp"),
    SYSCALLNAME(flock, 2, "nn"),
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
    SYSCALLNAME(times, 1, "p"),
    SYSCALLNAME(gettimeofday, 2, "pp"),
    SYSCALLNAME(settimeofday, 2, "pp"),

    SYSCALLNAME(uname, 1, "p"),
    SYSCALLNAME(brk, 1, "p"),
    SYSCALLNAME(osf_getsysinfo, 6, "xxxxxx"),
    SYSCALLNAME(osf_setsysinfo, 6, "xxxxxx"),

    SYSCALLNAME(access, 2, "sx"),

    SYSCALLNAME(getxpid, 0, ""),
    SYSCALLNAME(gettid, 0, ""),
    //SYSCALLNAME(setuid, 1),
    //SYSCALLNAME(getuid, 0),
    //SYSCALLNAME(seteuid, 1),
    SYSCALLNAME(getxuid, 0, ""),
    //SYSCALLNAME(setgid, 1),
    SYSCALLNAME(getxgid, 0, ""),
    //SYSCALLNAME(getgid, 0),
    //SYSCALLNAME(setegid, 1),
    //SYSCALLNAME(getegid, 0),
    //SYSCALLNAME(setreuid, 2),
    //SYSCALLNAME(setregid, 2),
    SYSCALLNAME(getrusage, 2, "xp"),
    SYSCALLNAME(getrlimit, 2, "np"),
    SYSCALLNAME(setrlimit, 2, "np"),
    SYSCALLNAME(exit_group, 0, ""),
    SYSCALLNAME(dup, 1, "n"),

    SYSCALLNAME(rt_sigaction, 3, "xxx"),
    SYSCALLNAME(osf_sigprocmask, 3, "xxx"),
    SYSCALLNAME(kill, 2, "xx"),
    SYSCALLNAME(tgkill, 3, "xxx"),
};
#undef SYSCALLNAME

} // namespace {
#endif // #ifdef SYSCALL_DEBUG


void AlphaLinuxSyscallConv::Execute(OpEmulationState* opState)
{
#ifdef SYSCALL_DEBUG 
    static std::ofstream log("syscall.log", std::ios_base::out);

    log << dec << GetVirtualSystem()->GetInsnTick() << "\t";
    log << dec << GetArg(0) << " ";
    for (size_t i = 0; i < sizeof(syscallTable)/sizeof(syscallTable[0]); i ++) {
        if (syscallTable[i].syscallNo == GetArg(0)) {
            log << "\t" << syscallTable[i].name << "(";
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
    case syscall_id_mprotect:
        syscall_mprotect(opState);
        break;
    case syscall_id_uname:
        syscall_uname(opState);
        break;
    case syscall_id_fcntl:
        syscall_fcntl(opState);
        break;
    case syscall_id_brk:
        syscall_brk(opState);
        break;
    case syscall_id_osf_getsysinfo:
        syscall_getsysinfo(opState);
        break;
    case syscall_id_osf_setsysinfo:
        syscall_setsysinfo(opState);
        break;
    case syscall_id_getxpid:
    case syscall_id_gettid:
        syscall_getpid(opState);
        break;
    //case syscall_id_setuid:
    //  syscall_setuid(opState);
    //  break;
    //case syscall_id_getuid:
    //  syscall_getuid(opState);
    //  break;
    //case syscall_id_setgid:
    //  syscall_setgid(opState);
    //  break;
    case syscall_id_getxgid:        // egid -> a4 (!), gid -> a3, error -> v0 なので割と大きな改造をしないと実装できない
        syscall_getegid(opState);
    //  break;
    //case syscall_id_seteuid:
    //  syscall_seteuid(opState);
    //  break;
    case syscall_id_getxuid:        // euid -> a4 (!), uid -> a3, error -> v0 なので割と大きな改造をしないと実装できない
        syscall_geteuid(opState);
        break;
    //case syscall_id_setegid:
    //  syscall_setegid(opState);
    //  break;
    //case syscall_id_getegid:
    //  syscall_getegid(opState);
    //  break;
    //case syscall_id_setregid:
    //  syscall_setregid(opState);
    //  break;
    //case syscall_id_setreuid:
    //  syscall_setreuid(opState);
    //  break;
    case syscall_id_access:
        syscall_access(opState);
        break;
    case syscall_id_stat:
    case syscall_id_stat64:
        syscall_stat64(opState);
        break;
    case syscall_id_lstat:
    case syscall_id_lstat64:
        syscall_lstat64(opState);
        break;
    case syscall_id_fstat:
    case syscall_id_fstat64:
        syscall_fstat64(opState);
        break;
    case syscall_id_ioctl:
        syscall_ioctl(opState);
        break;
    //case syscall_id_readlink:
    //  syscall_readlink(opState);
    //  break;
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

    // signal functions
    case syscall_id_rt_sigaction:
    case syscall_id_osf_sigprocmask:
        syscall_ignore(opState);
        break;
    case syscall_id_kill:
        syscall_kill(opState);
        break;
    case syscall_id_tgkill:
        syscall_tgkill(opState);
        break;

    // gcc が stack の拡張に使用するだけ
    case syscall_id_getrlimit:
        syscall_ignore(opState);
        break;
    case syscall_id_setrlimit:
        syscall_ignore(opState);
        break;

    case syscall_id_mkdir:
        syscall_mkdir(opState);
        break;

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

//
// 共通の実装
//


namespace {
    //struct alpha_stat64
    //{
    //  u64 alpha_stdev;
    //  u64 alpha_stino;        
    //  u64 alpha_strdev;
    //  s64 alpha_stsize;
    //  u64 alpha_stblocks;
    //  u32 alpha_stmode;
    //  u32 alpha_stuid;
    //  u32 alpha_stgid;
    //  u32 alpha_stblksize;
    //  u32 alpha_stnlink;
    //  s32 pad0;
    //  s64 alpha_statime;
    //  s64 pad1;
    //  s64 alpha_stmtime;
    //  s64 pad2;
    //  s64 alpha_stctime;
    //  s64 pad3;
    //  s64 unused[3];
    //};

    const int ALPHA_CLK_TCK = 1024;
    const int ALPHA_MREMAP_MAYMOVE = 1;
    const int ALPHA_MAP_NORESERVE = 0x10000;
    const int ALPHA_MAP_ANONYMOUS = 0x10;
}


// <FIXME>
// http://h50146.www5.hp.com/products/software/oe/tru64unix/manual/v51a_ref/HTML/MAN/MAN2/0120____.HTM
void AlphaLinuxSyscallConv::syscall_getsysinfo(OpEmulationState* opState)
{
    u64 kind = GetArg(1);
    u64 buffer = GetArg(2);

    switch (kind) {
    case 45: // GSI_IEEE_FP_CONTROL
    {
        EmuMemAccess access( buffer, sizeof(u64), (u64)0x137f );
        GetMemorySystem()->WriteMemory( &access );      // write default value
        SetResult(true, 0);
        break;
    }
    default:
        SetResult(false, EINVAL);
        break;
    }
}

// http://h50146.www5.hp.com/products/software/oe/tru64unix/manual/v51a_ref/HTML/MAN/MAN2/0185____.HTM
void AlphaLinuxSyscallConv::syscall_setsysinfo(OpEmulationState* opState)
{
    u64 kind = GetArg(1);
//  u64 buffer = GetArg(2);

    switch (kind) {
    case 14:    // SSI_IEEE_FP_CONTROL  動的丸めモード以外(FP例外・ステータス)を設定
        SetResult(true, 0);
        break;
    default:
        SetResult(false, EINVAL);
        break;
    }
}

//
// conversion
//

int AlphaLinuxSyscallConv::Get_MAP_ANONYMOUS()
{
    return ALPHA_MAP_ANONYMOUS;
}

int AlphaLinuxSyscallConv::Get_MREMAP_MAYMOVE()
{
    return ALPHA_MREMAP_MAYMOVE;
}

int AlphaLinuxSyscallConv::Get_CLK_TCK()
{
    return ALPHA_CLK_TCK;
}

//void AlphaLinuxSyscallConv::write_stat64(u64 dest, const HostStat &src)
//{
//  static u32 AlphaLinuxSyscallConst_host_st_mode[] =
//  {
//      POSIX_S_IFDIR,
//      POSIX_S_IFCHR,
//      POSIX_S_IFREG,
//      POSIX_S_IFIFO,
//  };
//  static u32 AlphaLinuxSyscallConst_alpha_st_mode[] =
//  {
//      0040000, // _S_IFDIR
//      0020000, // _S_IFCHR
//      0100000, // _S_IFREG
//      0010000, // _S_IFIFO
//  };
//  static int AlphaLinuxSyscallConst_st_mode_size = sizeof(AlphaLinuxSyscallConst_host_st_mode)/sizeof(AlphaLinuxSyscallConst_host_st_mode[0]);
//  SyscallConstConvEnum conv(
//      AlphaLinuxSyscallConst_host_st_mode,
//      AlphaLinuxSyscallConst_alpha_st_mode, 
//      AlphaLinuxSyscallConst_st_mode_size);
//
//  TargetBuffer buf(GetMemorySystem(), dest, sizeof(alpha_stat64));
//  alpha_stat64* a_buf = static_cast<alpha_stat64*>(buf.Get());
//  memset(a_buf, 0, sizeof(alpha_stat64));
//
//  a_buf->alpha_stdev = src.st_dev;
//  a_buf->alpha_stino = src.st_ino;
//  a_buf->alpha_strdev = src.st_rdev;
//  a_buf->alpha_stsize = src.st_size;
//  a_buf->alpha_stuid = src.st_uid;
//  a_buf->alpha_stgid = src.st_gid;
//  a_buf->alpha_stnlink = src.st_nlink;
//  a_buf->alpha_statime = src.st_atime;
//  a_buf->alpha_stmtime = src.st_mtime;
//  a_buf->alpha_stctime = src.st_ctime;
//
//#if defined(HOST_IS_WINDOWS) || defined(HOST_IS_CYGWIN)
//  a_buf->alpha_stmode = conv.TargetToHost(src.st_mode);
//  a_buf->alpha_stblocks = src.st_size;
//  a_buf->alpha_stblksize = 1;
//#else
//  a_buf->alpha_stblocks = src.st_blocks;
//  a_buf->alpha_stmode = src.st_mode;
//  a_buf->alpha_stblksize = src.st_blksize;
//#endif
//}


u32 AlphaLinuxSyscallConv::OpenFlagTargetToHost(u32 flag)
{
    static u32 AlphaLinuxSyscallConst_host_open_flags[] =
    {
      POSIX_O_RDONLY, POSIX_O_WRONLY, POSIX_O_RDWR,
      POSIX_O_APPEND,
      POSIX_O_CREAT,
      POSIX_O_EXCL,
//    O_NOCTTY,
//    O_NONBLOCK,
//    O_SYNC,
      POSIX_O_TRUNC,
    };
    static u32 AlphaLinuxSyscallConst_alpha_open_flags[] =
    {
      00, 01, 02,   // O_RDONLY, O_WRONLY, O_RDWR,
      00010,    // O_APPEND
      01000,    // O_CREAT
      04000,    // O_EXCL
//    010000,   // O_NOCTTY
//    00004,    // O_NONBLOCK
//    040000,   // O_SYNC
      02000,    // O_TRUNC
    };
    static int AlphaLinuxSyscallConst_open_flags_size = sizeof(AlphaLinuxSyscallConst_host_open_flags)/sizeof(AlphaLinuxSyscallConst_host_open_flags[0]);

    SyscallConstConvBitwise conv(
        AlphaLinuxSyscallConst_host_open_flags,
        AlphaLinuxSyscallConst_alpha_open_flags, 
        AlphaLinuxSyscallConst_open_flags_size);

    return conv.TargetToHost(flag);
}

