// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Masahiro Goshima.
// Copyright (c) 2005-2018 Ryota Shioya.
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

#include "Emu/RISCV64Linux/RISCV64LinuxSyscallConv.h"

#include "Env/Env.h"
#include "SysDeps/posix.h"
#include "SysDeps/Endian.h"

#include "Emu/Utility/System/ProcessState.h"
#include "Emu/Utility/System/Memory/MemorySystem.h"
#include "Emu/Utility/System/Memory/MemoryUtility.h"
#include "Emu/Utility/System/VirtualSystem.h"
#include "Emu/Utility/System/Syscall/SyscallConstConv.h"
#include "Emu/Utility/OpEmulationState.h"

#include "Emu/RISCV64Linux/RISCV64LinuxSyscallID.h"

#define SYSCALL_DEBUG

using namespace std;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::RISCV64Linux;
using namespace Onikiri::POSIX;


RISCV64LinuxSyscallConv::RISCV64LinuxSyscallConv(ProcessState* processState) : 
    Linux64SyscallConv(processState)
{
}

RISCV64LinuxSyscallConv::~RISCV64LinuxSyscallConv()
{
}

#ifdef SYSCALL_DEBUG
#define SYSCALLNAME(name, argcnt, argtempl) {syscall_id_##name, #name, argcnt, argtempl}

namespace {

    typedef s64 RISCV64_time_t;

    struct riscv64_timespec
    {
        RISCV64_time_t tv_sec;
        RISCV64_time_t tv_nsec;
    };
    
    // "stat" structure is different for each ISA, so specific one is defined here
    struct riscv64_stat
    {
        s64 st_dev;
        s64 st_ino;
        s32 st_mode;
        s32 st_nlink;
        s32 st_uid;
        s32 st_gid;
        s64 st_rdev;
        s64 __pad1;
        s64 st_size;
        s32 st_blksize;
        s32 __pad2;
        s64 st_blocks;
        riscv64_timespec st_atim;
        riscv64_timespec st_mtim;
        riscv64_timespec st_ctim;
        s32 __glibc_reserved[2];
    };
    /*
    struct RISCV64_iovec
    {
        u64 linux64_iov_base;
        u64 linux64_iov_len;
    };

    struct RISCV64_tms
    {
        s64 linux64_tms_utime;
        s64 linux64_tms_stime;
        s64 linux64_tms_cutime;
        s64 linux64_tms_cstime;
    };

    struct RISCV64_timeval {
        s64 linux64_tv_sec;
        s64 linux64_tv_usec;
    };
    */


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
    SYSCALLNAME(getcwd, 2, "px"),
    SYSCALLNAME(close, 1, "n"),
    SYSCALLNAME(lseek, 3, "nxn"),
    SYSCALLNAME(read, 3, "npn"),
    SYSCALLNAME(write, 3, "npn"),
    SYSCALLNAME(fstat, 2, "np"),
    SYSCALLNAME(exit, 0, ""),
    SYSCALLNAME(exit_group, 0, ""),
    SYSCALLNAME(gettimeofday, 2, "pp"),
    SYSCALLNAME(brk, 1, "p"),
    SYSCALLNAME(open, 3, "sxx"),
    SYSCALLNAME(openat, 4, "nsxx"),
    SYSCALLNAME(unlink, 1, "s"),
    SYSCALLNAME(stat, 2, "sp"),
    SYSCALLNAME(uname, 1, "p"),
    SYSCALLNAME(writev, 3, "npn"),
    SYSCALLNAME(mmap, 6, "pxxxnx"),
    SYSCALLNAME(munmap, 2, "px"),
    SYSCALLNAME(mremap, 4, "pxxx"),
    SYSCALLNAME(readlinkat, 4, "nssn"),
    SYSCALLNAME(sigaction, 3 , "npp"),
    SYSCALLNAME(fstatat, 4, "nspn"),
    SYSCALLNAME(ioctl, 3, "xxx"),
    SYSCALLNAME(fcntl, 3, "nnp"),
    SYSCALLNAME(faccessat, 4, "npnn"),
    SYSCALLNAME(ftruncate, 2, "nn"),
    SYSCALLNAME(unlinkat, 3, "nsn"),
    SYSCALLNAME(clock_gettime, 2, "np"),
    SYSCALLNAME(getrusage, 2, "xp"),
    SYSCALLNAME(sysinfo, 1, "p"),
    SYSCALLNAME(getrlimit, 2, "np"),
    SYSCALLNAME(setrlimit, 2, "np"),
    SYSCALLNAME(prlimit64, 4, "nnpp"),
    SYSCALLNAME(chdir, 1, "s"),
    SYSCALLNAME(pipe2, 1, "pn"),
    SYSCALLNAME(clone , 5, "npppp"),
    SYSCALLNAME(times, 1, "p"),
    SYSCALLNAME(getdents64, 3, "npn"),
    SYSCALLNAME(getuid, 0, ""),
    SYSCALLNAME(getgid, 0, ""),
    SYSCALLNAME(geteuid, 0, ""),
    SYSCALLNAME(getegid, 0, ""),
    SYSCALLNAME(getpid, 0, ""),
    SYSCALLNAME(mkdirat, 3, "nsx"),
    SYSCALLNAME(renameat, 4, "nsns"),
    SYSCALLNAME(renameat2, 5, "nsnsn"),
    SYSCALLNAME(set_tid_address, 1, "p"),
    SYSCALLNAME(madvise, 3, "pxn"),

    /*
    SYSCALLNAME(readv, 3, "npn"),
    SYSCALLNAME(writev, 3, "npn"),
    SYSCALLNAME(lstat, 2, "sp"),
    //SYSCALLNAME(stat64, 2, "sp"),
    //SYSCALLNAME(lstat64, 2, "sp"),
    //SYSCALLNAME(fstat64, 2, "np"),
    SYSCALLNAME(truncate, 2, "px"),
    SYSCALLNAME(ftruncate, 2, "nx"),
    SYSCALLNAME(fcntl, 3, "nnp"),
    SYSCALLNAME(fchdir, 1, "n"),
    SYSCALLNAME(mkdir, 2, "sx"),
    SYSCALLNAME(rmdir, 1, "s"),
    SYSCALLNAME(readlink, 3, "spx"),
    SYSCALLNAME(link, 2, "ss"),
    SYSCALLNAME(mprotect, 3, "pxx"),
    SYSCALLNAME(chmod, 2, "sx"),
    SYSCALLNAME(time, 1, "p"),
    //SYSCALLNAME(settimeofday, 2, "pp"),

    SYSCALLNAME(uname, 1, "p"),

    SYSCALLNAME(access, 2, "sx"),

    SYSCALLNAME(gettid, 0, ""),
    SYSCALLNAME(setuid, 1, "x"),
    //SYSCALLNAME(seteuid, 1, "x"),
    SYSCALLNAME(setgid, 1, "x"),
    //SYSCALLNAME(setegid, 1, "x"),
    //SYSCALLNAME(setreuid, 2),
    //SYSCALLNAME(setregid, 2),
    SYSCALLNAME(dup, 1, "n"),

    SYSCALLNAME(rt_sigaction, 3, "xxx"),
    SYSCALLNAME(rt_sigprocmask, 3, "xxx"),
    SYSCALLNAME(kill, 2, "xx"),
    SYSCALLNAME(tgkill, 3, "xxx"),
*/
};
#undef SYSCALLNAME

} // namespace {
#endif // #ifdef SYSCALL_DEBUG


void RISCV64LinuxSyscallConv::Execute(OpEmulationState* opState)
{
#ifdef SYSCALL_DEBUG 
    //static std::ofstream log("syscall.log", std::ios_base::out | std::ios_base::trunc);
    auto& log = std::cout;

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
    case syscall_id_getcwd:
        syscall_getcwd(opState);
        break;
    case syscall_id_ioctl:
        syscall_ioctl(opState);
        break;
    case syscall_id_close:
        syscall_close(opState);
        break;

    case syscall_id_getdents64:
        syscall_getdents64(opState);
        break;
    case syscall_id_lseek:
        syscall_lseek(opState);
        break;
    case syscall_id_read:
        syscall_read(opState);
        break;
    case syscall_id_write:
        syscall_write(opState);
        break;
    case syscall_id_fstat:
        syscall_fstat64(opState);
        break;

    case syscall_id_chdir:
        syscall_chdir(opState);
        break;

    case syscall_id_exit:
    case syscall_id_exit_group:
        syscall_exit(opState);
        break;
    case syscall_id_gettimeofday:
        syscall_gettimeofday(opState);
        break;
    case syscall_id_brk:
        syscall_brk(opState);
        break;
    case syscall_id_open:
        syscall_open(opState);
        break;
    case syscall_id_unlink:
        syscall_unlink(opState);
        break;
    case syscall_id_stat:
        syscall_stat64(opState);
        break;

    case syscall_id_sigaction:
        syscall_sigaction(opState);
        break;

    case syscall_id_uname:
        syscall_uname(opState);
        break;

    case syscall_id_openat:
        syscall_openat(opState);
        break;

    case syscall_id_writev:
        syscall_writev(opState);
        break;

    case syscall_id_readlinkat:
        syscall_readlinkat(opState);
        break;
    case syscall_id_fstatat:
        syscall_fstatat64(opState);
        break;
    case syscall_id_unlinkat:
        syscall_unlinkat(opState);
        break;

    case syscall_id_munmap:
        syscall_munmap(opState);
        break;
    case syscall_id_mremap:
        syscall_mremap(opState);
        break;
    case syscall_id_mmap:
        syscall_mmap(opState);
        break;

    case syscall_id_geteuid:
        syscall_geteuid(opState);
        break;

    case syscall_id_getpid:
    case syscall_id_getppid:
        syscall_getpid(opState);
        break;

    case syscall_id_getuid:
        syscall_getuid(opState);
        break;

    case syscall_id_getegid:
        syscall_getegid(opState);
        break;

    case syscall_id_getgid:
        syscall_getgid(opState);
        break;

    case syscall_id_fcntl:
        syscall_fcntl(opState);
        break;

    case syscall_id_faccessat:
        syscall_faccessat(opState);
        break;

    case syscall_id_ftruncate:
        syscall_ftruncate(opState);
        break;

    case syscall_id_clock_gettime:
        syscall_clock_gettime(opState);
        break;
    case syscall_id_times:
        syscall_times(opState);
        break;

    // gcc が実行時間の取得に使用
    case syscall_id_getrusage:
        syscall_ignore(opState);
        break;

    case syscall_id_sysinfo:
        syscall_sysinfo(opState);
        break;

    // gcc が stack の拡張に使用するだけ
    case syscall_id_getrlimit:
        syscall_ignore(opState);
        break;
    case syscall_id_setrlimit:
        syscall_ignore(opState);
        break;
    case syscall_id_prlimit64:
        syscall_ignore(opState);
        break;

    case syscall_id_mkdirat:
        syscall_mkdirat(opState);
        break;
    case syscall_id_renameat:
        syscall_renameat(opState);
        break;
    case syscall_id_renameat2:
        syscall_renameat2(opState);
        break;

    case syscall_id_set_tid_address:
        // There is nothing to do since syscall_clone is not supported.
        // Returns TID.
        syscall_gettid(opState);
        break;

    case syscall_id_madvise:
        // There is nothing to do since this system call is just advice.
        syscall_ignore(opState);
        break;

    /*
    case syscall_id_readv:
        syscall_readv(opState);
        break;
    case syscall_id_writev:
        syscall_writev(opState);
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
    case syscall_id_lstat:
//  case syscall_id_lstat64:
        syscall_lstat64(opState);
        break;
    //case syscall_id_readlink:
    //  syscall_readlink(opState);
    //  break;
    case syscall_id_time:
        syscall_time(opState);
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


    //case syscall_id_mkdir:
    //  syscall_mkdir(opState);
    //  break;


*/
    case syscall_id_pipe2:
        THROW_RUNTIME_ERROR("Unsupported syscall 'pipe2' is called");
        break;

    case syscall_id_clone:
        THROW_RUNTIME_ERROR("Unsupported syscall 'clone' is called");
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
    const int RISCV64_CLK_TCK = 100;
    const int RISCV64_MREMAP_MAYMOVE = 1;
    const int RISCV64_MAP_ANONYMOUS = 0x20;
}

int RISCV64LinuxSyscallConv::Get_MAP_ANONYMOUS()
{
    return RISCV64_MAP_ANONYMOUS;
}

int RISCV64LinuxSyscallConv::Get_MREMAP_MAYMOVE()
{
    return RISCV64_MREMAP_MAYMOVE;
}

int RISCV64LinuxSyscallConv::Get_CLK_TCK()
{
    return RISCV64_CLK_TCK;
}

// LinuxSyscallConv calls this method when open() is called
// because these flags seem to depend on each architecture?
u32 RISCV64LinuxSyscallConv::OpenFlagTargetToHost(u32 flag)
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
        POSIX_O_DIRECTORY,
    };

    // see bits/fcntl.h
    // or linux-headers/include/asm-generic/fcntl.h
    static u32 riscv64_open_flags[] =
    {
        00, 01, 02, // O_RDONLY, O_WRONLY, O_RDWR,
        02000,  // O_APPEND
        00100,  // O_CREAT
        00200,  // O_EXCL
        //00400,    // O_NOCTTY
        //04000,    // O_NONBLOCK
        //010000,   // O_SYNC
        01000,      // O_TRUNC
        00200000    // O_DIRECTORY	
    };
    static int open_flags_size = sizeof(host_open_flags)/sizeof(host_open_flags[0]);

    SyscallConstConvBitwise conv(
        host_open_flags,
        riscv64_open_flags,
        open_flags_size
    );

    return conv.TargetToHost(flag);
}

void RISCV64LinuxSyscallConv::write_stat64(u64 dest, const HostStat &src)
{
    static u32 host_st_mode[] =
    {
        POSIX_S_IFDIR,
        POSIX_S_IFCHR,
        POSIX_S_IFREG,
        POSIX_S_IFIFO,
    };
    static u32 RISCV64_st_mode[] =
    {
        0040000, // _S_IFDIR
        0020000, // _S_IFCHR
        0100000, // _S_IFREG
        0010000, // _S_IFIFO
    };
    static int st_mode_size = sizeof(host_st_mode) / sizeof(host_st_mode[0]);
    SyscallConstConvBitwise conv(
        host_st_mode,
        RISCV64_st_mode,
        st_mode_size
    );

    TargetBuffer buf(GetMemorySystem(), dest, sizeof(riscv64_stat));
    riscv64_stat* t_buf = static_cast<riscv64_stat*>(buf.Get());
    memset(t_buf, 0, sizeof(riscv64_stat));

    t_buf->st_dev = src.st_dev;
    t_buf->st_ino = src.st_ino;
    t_buf->st_rdev = src.st_rdev;
    t_buf->st_size = src.st_size;
    t_buf->st_uid = src.st_uid;
    t_buf->st_gid = src.st_gid;
    t_buf->st_nlink = src.st_nlink;
    /*
    t_buf->st_atime = src.st_atime;
    t_buf->st_mtime = src.st_mtime;
    t_buf->st_ctime = src.st_ctime;
    */

    // st_blksize : 効率的にファイル・システムIO が行える"好ましい"ブロック・サイズ
    //              CentOS4 では32768
    // st_blocks  : ファイルに割り当てられた512B の数
    //              CentOS4 では8 刻みになる？

#if defined(HOST_IS_WINDOWS) || defined(HOST_IS_CYGWIN)
    static const int BLOCK_UNIT = 512 * 8;
    t_buf->st_mode = conv.HostToTarget(src.st_mode);
    t_buf->st_blocks = (src.st_size + BLOCK_UNIT - 1) / BLOCK_UNIT * 8;
    t_buf->st_blksize = 32768;
#else
    t_buf->st_blocks = src.st_blocks;
    t_buf->st_mode = src.st_mode;
    t_buf->st_blksize = src.st_blksize;
#endif

    // hostのstat(src)の要素とtargetのstatの要素のサイズが異なるかもしれないので，一旦targetのstatに代入してからエンディアンを変換する
    bool bigEndian = GetMemorySystem()->IsBigEndian();
    EndianHostToSpecifiedInPlace(t_buf->st_dev, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->st_ino, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->st_rdev, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->st_size, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->st_uid, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->st_gid, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->st_nlink, bigEndian);
    /*
    EndianHostToSpecifiedInPlace(t_buf->st_atime, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->st_mtime, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->st_ctime, bigEndian);
    */
    EndianHostToSpecifiedInPlace(t_buf->st_mode, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->st_blocks, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->st_blksize, bigEndian);
}



