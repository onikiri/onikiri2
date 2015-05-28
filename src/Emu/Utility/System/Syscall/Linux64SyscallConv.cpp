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

#include "Emu/Utility/System/Syscall/Linux64SyscallConv.h"

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



// Linux64SyscallConv
Linux64SyscallConv::Linux64SyscallConv(ProcessState* processState)
    : m_processState(processState)
{
    m_args.resize(MaxArgCount);
    m_results.resize(MaxResultCount);
}

Linux64SyscallConv::~Linux64SyscallConv()
{
}

void Linux64SyscallConv::SetArg(int index, u64 value)
{
    m_args.at(index) = value;
}

u64 Linux64SyscallConv::GetResult(int index)
{
    return m_results.at(index);
}

void Linux64SyscallConv::SetResult(bool success, u64 result)
{
    m_results[RetValueIndex] = result;
    m_results[ErrorFlagIndex] = success ? 0 : 1;    // error flag
}

void Linux64SyscallConv::SetSystem(SystemIF* system)
{
    m_simulatorSystem = system;
}


//
// 共通の実装
//


namespace {
    struct linux64_stat64
    {
        u64 linux64_stdev;
        u64 linux64_stino;      
        u64 linux64_strdev;
        s64 linux64_stsize;
        u64 linux64_stblocks;
        u32 linux64_stmode;
        u32 linux64_stuid;
        u32 linux64_stgid;
        u32 linux64_stblksize;
        u32 linux64_stnlink;
        s32 pad0;
        s64 linux64_statime;
        s64 pad1;
        s64 linux64_stmtime;
        s64 pad2;
        s64 linux64_stctime;
        s64 pad3;
        s64 unused[3];
    };

    struct linux64_iovec
    {
        u64 linux64_iov_base;
        u64 linux64_iov_len;
    };

    struct linux64_tms
    {
        s64 linux64_tms_utime;
        s64 linux64_tms_stime;
        s64 linux64_tms_cutime;
        s64 linux64_tms_cstime;
    };

    struct linux64_timeval {
        s64 linux64_tv_sec;
        s64 linux64_tv_usec;
    };

    typedef s64 linux64_time_t;

    //const int LINUX_F_DUPFD = 0;
    const int LINUX_F_GETFD = 1;
    const int LINUX_F_SETFD = 2;

    const int LINUX_F_OK = 0;
    const int LINUX_X_OK = 0;
    const int LINUX_W_OK = 2;
    const int LINUX_R_OK = 4;

    const int LINUX_SEEK_SET = 0;
    const int LINUX_SEEK_CUR = 1;
    const int LINUX_SEEK_END = 2;
}



void Linux64SyscallConv::syscall_ignore(OpEmulationState* opState)
{
    m_results[RetValueIndex] = (u64)0;
    m_results[ErrorFlagIndex] = (u64)0;
}

void Linux64SyscallConv::syscall_exit(OpEmulationState* opState)
{
    opState->SetTakenPC(0);
    opState->SetTaken(true);

    m_simulatorSystem->NotifyProcessTermination(opState->GetPID());
}

void Linux64SyscallConv::syscall_getuid(OpEmulationState* opState)
{
    SetResult(true, (u64)posix_getuid());
}

void Linux64SyscallConv::syscall_geteuid(OpEmulationState* opState)
{
    SetResult(true, (u64)posix_geteuid());
}

void Linux64SyscallConv::syscall_getgid(OpEmulationState* opState)
{
    SetResult(true, (u64)posix_getgid());
}

void Linux64SyscallConv::syscall_getegid(OpEmulationState* opState)
{
    SetResult(true, (u64)posix_getegid());
}

void Linux64SyscallConv::syscall_brk(OpEmulationState* opState)
{
    if (m_args[1] == 0) {
        SetResult(true, GetMemorySystem()->Brk(0));
        return;
    }

    if (GetMemorySystem()->Brk(m_args[1]) == (u64)-1) {
        SetResult(false, ENOMEM);
    }
    else {
        // Linux のbrkシステムコールは，成功時に新しいbreakを返す
        SetResult(true, GetMemorySystem()->Brk(0));
    }
}

void Linux64SyscallConv::syscall_mmap(OpEmulationState* opState)
{
    u64 result;
    if (!(m_args[4] & Get_MAP_ANONYMOUS())) {
        result = (u64)-1;
    }
    else {
        result = GetMemorySystem()->MMap(m_args[1], m_args[2]);
    }

    if (result == (u64)-1)
        SetResult(false, ENOMEM);   // <TODO> errnoの設定
    else
        SetResult(true, result);
}
void Linux64SyscallConv::syscall_mremap(OpEmulationState* opState)
{
    u64 result = GetMemorySystem()->MRemap(m_args[1], m_args[2], m_args[3], (m_args[4] & Get_MREMAP_MAYMOVE()) != 0);
    if (result == (u64)-1)
        SetResult(false, ENOMEM);
    else
        SetResult(true, result);
}
void Linux64SyscallConv::syscall_munmap(OpEmulationState* opState)
{
    int result = GetMemorySystem()->MUnmap(m_args[1], m_args[2]);
    if (result == -1)
        SetResult(false, EINVAL);
    else
        SetResult(true, (u64)result);
}
void Linux64SyscallConv::syscall_mprotect(OpEmulationState* opState)
{
    SetResult(true, (u64)0);
}

void Linux64SyscallConv::syscall_uname(OpEmulationState* opState)
{
    // linux
    struct utsname_linux
    {
        char sysname[65];
        char nodename[65];
        char release[65];
        char version[65];
        char machine[65];
    } utsname;

    memset(&utsname, 0, sizeof(utsname));
    strcpy(utsname.release, "2.6.23");

    GetMemorySystem()->MemCopyToTarget(m_args[1], &utsname, sizeof(utsname));

    SetResult(true, 0);
}



void Linux64SyscallConv::syscall_getpid(OpEmulationState* opState)
{
    SetResult(true, (u64)posix_getpid());
}

void Linux64SyscallConv::syscall_kill(OpEmulationState* opState)
{
    kill_helper(opState, (int)m_args[1], (int)m_args[2]);
}

void Linux64SyscallConv::syscall_tgkill(OpEmulationState* opState)
{
    ASSERT(m_args[1] == m_args[2]);
    kill_helper(opState, (int)m_args[1], (int)m_args[3]);
}

void Linux64SyscallConv::kill_helper(OpEmulationState* opState, int arg_pid, int arg_sig)
{
    if (arg_pid == posix_getpid()) {
        if (arg_sig == 6)   // SIGABRT
            g_env.Print("target program aborted.\n");
        else
            g_env.Print("signaling with kill not implemented yet.\n");
    }
    else {
        g_env.Print("target program attempted to kill another process.\n");
    }

    syscall_exit(opState);
}

void Linux64SyscallConv::syscall_getcwd(OpEmulationState* opState)
{
    int bufSize = (int)m_args[2];

    TargetBuffer buf(GetMemorySystem(), m_args[1], bufSize+1);
    void *result = GetVirtualSystem()->GetCWD(static_cast<char*>(buf.Get()), bufSize);
    if (result == NULL) {
        SetResult(false, GetVirtualSystem()->GetErrno());
    }
    else {
        SetResult(true, m_args[1]);
    }
}

void Linux64SyscallConv::syscall_open(OpEmulationState* opState)
{
    std::string fileName = StrCpyToHost( GetMemorySystem(), m_args[1] );
    int result = GetVirtualSystem()->Open( 
        fileName.c_str(), 
        (int)OpenFlagTargetToHost( static_cast<u32>(m_args[2]) ) 
    );

    if (result == -1)
        SetResult(false, GetVirtualSystem()->GetErrno());
    else
        SetResult(true, result);
}

void Linux64SyscallConv::syscall_close(OpEmulationState* opState)
{
    int result = GetVirtualSystem()->Close((int)m_args[1]);
    if (result == -1)
        SetResult(false, GetVirtualSystem()->GetErrno());
    else
        SetResult(true, result);
}

void Linux64SyscallConv::syscall_dup(OpEmulationState* opState)
{
    int result = GetVirtualSystem()->Dup((int)m_args[1]);
    if (result == -1)
        SetResult(false, GetVirtualSystem()->GetErrno());
    else
        SetResult(true, result);
}

void Linux64SyscallConv::syscall_read(OpEmulationState* opState)
{
    unsigned int bufSize = (unsigned int)m_args[3];
    TargetBuffer buf(GetMemorySystem(), m_args[2], bufSize);
    int result = GetVirtualSystem()->Read((int)m_args[1], buf.Get(),    bufSize);
    if (result == -1) {
        SetResult(false, GetVirtualSystem()->GetErrno());
    }
    else {
        m_simulatorSystem->NotifySyscallReadFileToMemory(Addr(opState->GetPID(), opState->GetTID(), m_args[2]), bufSize);
        SetResult(true, result);
    }
}

void Linux64SyscallConv::syscall_write(OpEmulationState* opState)
{
    unsigned int bufSize = (unsigned int)m_args[3];
    TargetBuffer buf(GetMemorySystem(), m_args[2], bufSize);
    int result = GetVirtualSystem()->Write((int)m_args[1], buf.Get(), bufSize);
    if (result == -1) {
        SetResult(false, GetVirtualSystem()->GetErrno());
    }
    else {
        m_simulatorSystem->NotifySyscallWriteFileFromMemory(Addr(opState->GetPID(), opState->GetTID(), m_args[2]), bufSize);
        SetResult(true, result);
    }
}


void Linux64SyscallConv::syscall_readv(OpEmulationState* opState)
{
    int iovcnt = (int)m_args[3];
    s64 result = 0;
    const TargetBuffer iovecBuf(GetMemorySystem(), m_args[2], iovcnt*sizeof(linux64_iovec), true);
    const linux64_iovec* iovec = static_cast<const linux64_iovec*>(iovecBuf.Get());

    for (int i = 0; i < iovcnt; i ++) {
        u64 iov_base = EndianSpecifiedToHost(iovec->linux64_iov_base, GetMemorySystem()->IsBigEndian());
        u64 iov_len = EndianSpecifiedToHost(iovec->linux64_iov_len, GetMemorySystem()->IsBigEndian());

        TargetBuffer buf(GetMemorySystem(), iov_base, (size_t)iov_len);
        int bytesRead = GetVirtualSystem()->Read((int)m_args[1], buf.Get(), (unsigned int)iov_len);

        if (result == -1 || bytesRead == -1 || (u64)bytesRead != iov_len) {
            SetResult(false, GetVirtualSystem()->GetErrno());
            result = -1;
            break;
        }
        else {
            m_simulatorSystem->NotifySyscallReadFileToMemory(Addr(opState->GetPID(), opState->GetTID(), iov_base), iov_len);
            result += iov_len;
        }
        iovec++;
    }
    SetResult(true, result);
}

void Linux64SyscallConv::syscall_writev(OpEmulationState* opState)
{
    int iovcnt = (int)m_args[3];
    s64 result = 0;
    const TargetBuffer iovecBuf(GetMemorySystem(), m_args[2], iovcnt*sizeof(linux64_iovec), true);
    const linux64_iovec* iovec = static_cast<const linux64_iovec*>(iovecBuf.Get());

    for (int i = 0; i < iovcnt; i ++) {
        u64 iov_base = EndianSpecifiedToHost(iovec->linux64_iov_base, GetMemorySystem()->IsBigEndian());
        u64 iov_len = EndianSpecifiedToHost(iovec->linux64_iov_len, GetMemorySystem()->IsBigEndian());

        TargetBuffer buf(GetMemorySystem(), iov_base, (size_t)iov_len);
        int bytesWritten = GetVirtualSystem()->Write((int)m_args[1], buf.Get(), (unsigned int)iov_len);

        if (result == -1 || bytesWritten == -1 || (u64)bytesWritten != iov_len) {
            SetResult(false, GetVirtualSystem()->GetErrno());
            result = -1;
            break;
        }
        else {
            m_simulatorSystem->NotifySyscallWriteFileFromMemory(Addr(opState->GetPID(), opState->GetTID(), iov_base), iov_len);
            result += iov_len;
        }
        iovec++;
    }
    SetResult(true, result);
}

void Linux64SyscallConv::syscall_lseek(OpEmulationState* opState)
{
    s64 result = GetVirtualSystem()->LSeek((int)m_args[1], m_args[2], (int)SeekWhenceTargetToHost((u32)m_args[3]));
    if (result == -1)
        SetResult(false, GetVirtualSystem()->GetErrno());
    else
        SetResult(true, (u64)result);
}

void Linux64SyscallConv::syscall_fstat64(OpEmulationState* opState)
{
    HostStat st;
    int result = GetVirtualSystem()->FStat((int)m_args[1], &st);
    if (result == -1) {
        SetResult(false, GetVirtualSystem()->GetErrno());
    }
    else {
#ifdef HOST_IS_WINDOWS
        /*
        st.st_rdev は Windows では st.st_dev と同じだが、
        (http://msdn.microsoft.com/ja-jp/library/14h5k7ff.aspx)
        Linux では特殊なファイルの場合ここの値が変わる。
        標準出力だとどうやら 0x8801 になるらしい？（要出典）
        この st_rdev で実行パスが変わることがあるため、ホストの入出力を使用する場合は
        とりあえず 0x8801 とする。
        （例えば mcf では if(st.st_rdev >> 4) で Copyright の書込み先を変えるようで、
        そのためにここの値が正しくないと実行パスが変わる）
        */
        if(GetVirtualSystem()->GetDelayUnlinker()->GetMapPath((int)m_args[1]) == "HostIO"){
            st.st_rdev = 0x8801;
        }
#endif
        write_stat64((u64)m_args[2], st);
        SetResult(true, result);
    }
}

void Linux64SyscallConv::syscall_stat64(OpEmulationState* opState)
{
    HostStat st;
    string path = StrCpyToHost( GetMemorySystem(), m_args[1] );
    int result = GetVirtualSystem()->Stat(path.c_str(), &st);
    if (result == -1) {
        SetResult(false, GetVirtualSystem()->GetErrno());
    }
    else {
        write_stat64((u64)m_args[2], st);
        SetResult(true, result);
    }
}

void Linux64SyscallConv::syscall_lstat64(OpEmulationState* opState)
{
    HostStat st;
    string path = StrCpyToHost( GetMemorySystem(), m_args[1] );
    int result = GetVirtualSystem()->LStat(path.c_str(), &st);
    if (result == -1) {
        SetResult(false, GetVirtualSystem()->GetErrno());
    }
    else {
        write_stat64((u64)m_args[2], st);
        SetResult(true, result);
    }
}

void Linux64SyscallConv::syscall_fcntl(OpEmulationState* opState)
{
    int cmd = (int)m_args[2];
    switch (cmd) {
    case LINUX_F_GETFD:
    case LINUX_F_SETFD:
        SetResult(true, 0);
        break;
    default:
        THROW_RUNTIME_ERROR("syscall_fcntl : unknown cmd");
    }
}

void Linux64SyscallConv::syscall_mkdir(OpEmulationState* opState)
{
    string path = StrCpyToHost( GetMemorySystem(), m_args[1] );
    int result = GetVirtualSystem()->MkDir(path.c_str(), (int)m_args[2]);
    if (result == -1) {
        SetResult(false, GetVirtualSystem()->GetErrno());
    }
    else {
        SetResult(true, result);
    }
}

void Linux64SyscallConv::syscall_access(OpEmulationState* opState)
{
    string path = StrCpyToHost( GetMemorySystem(), m_args[1] );
    int result = GetVirtualSystem()->Access( path.c_str(), (int)AccessModeTargetToHost((u32)m_args[2]) );
    if (result == -1)
        SetResult(false, GetVirtualSystem()->GetErrno());
    else
        SetResult(true, result);
}

void Linux64SyscallConv::syscall_unlink(OpEmulationState* opState)
{
    string path = StrCpyToHost( GetMemorySystem(), m_args[1] );
    int result = GetVirtualSystem()->Unlink( path.c_str() );
    if (result == -1)
        SetResult(false, GetVirtualSystem()->GetErrno());
    else
        SetResult(true, result);
}

void Linux64SyscallConv::syscall_rename(OpEmulationState* opState)
{
    string oldpath = StrCpyToHost( GetMemorySystem(), m_args[1] );
    string newpath = StrCpyToHost( GetMemorySystem(), m_args[2] );

    int result = GetVirtualSystem()->Rename( oldpath.c_str(), newpath.c_str() );
    if (result == -1)
        SetResult(false, GetVirtualSystem()->GetErrno());
    else
        SetResult(true, result);
}

void Linux64SyscallConv::syscall_truncate(OpEmulationState* opState)
{
    string path = StrCpyToHost( GetMemorySystem(), m_args[1] );
    int result = GetVirtualSystem()->Truncate( path.c_str(), (s64)m_args[2] );
    if (result == -1)
        SetResult(false, GetVirtualSystem()->GetErrno());
    else
        SetResult(true, result);
}

void Linux64SyscallConv::syscall_ftruncate(OpEmulationState* opState)
{
    int result = GetVirtualSystem()->FTruncate( (int)m_args[1], (s64)m_args[2] );
    if (result == -1)
        SetResult(false, GetVirtualSystem()->GetErrno());
    else
        SetResult(true, result);
}

void Linux64SyscallConv::syscall_time(OpEmulationState* opState)
{
    s64 result = GetVirtualSystem()->GetTime();
    if (m_args[1] != 0) {
        TargetBuffer buf(GetMemorySystem(), m_args[1], sizeof(linux64_time_t));
        linux64_time_t* ptime = static_cast<linux64_time_t*>(buf.Get());
        *ptime = EndianHostToSpecified(result, GetMemorySystem()->IsBigEndian());
    }
    SetResult(true, result);
}

void Linux64SyscallConv::syscall_times(OpEmulationState* opState)
{
    TargetBuffer buf(GetMemorySystem(), m_args[1], sizeof(linux64_tms));
    linux64_tms* tms_buf = static_cast<linux64_tms*>(buf.Get());

    s64 clk = GetVirtualSystem()->GetClock();
    u64 result = (u64)((double)clk / CLOCKS_PER_SEC * Get_CLK_TCK());
    tms_buf->linux64_tms_utime = EndianHostToSpecified(result, GetMemorySystem()->IsBigEndian());
    tms_buf->linux64_tms_stime = 0;
    tms_buf->linux64_tms_cutime = 0;
    tms_buf->linux64_tms_cstime = 0;

    SetResult(true, result);
}

void Linux64SyscallConv::syscall_gettimeofday(OpEmulationState* opState)
{
    if (m_args[2] != 0) {
        // timezone 引数はobsoleteなのでサポートしない
        SetResult(false, EINVAL);
        return;
    }
    TargetBuffer buf(GetMemorySystem(), m_args[1], sizeof(linux64_timeval));
    linux64_timeval* tv_buf = static_cast<linux64_timeval*>(buf.Get());

    s64 t = GetVirtualSystem()->GetTime();
    tv_buf->linux64_tv_sec = EndianHostToSpecified((u64)t, GetMemorySystem()->IsBigEndian());
    tv_buf->linux64_tv_usec = EndianHostToSpecified((u64)t * 1000 * 1000, GetMemorySystem()->IsBigEndian());

    SetResult(true, 0);
}

void Linux64SyscallConv::syscall_ioctl(OpEmulationState* opState)
{
    const int LINUX_TCGETS = 0x402c7413;

    // LINUX_TCGETSのとき，isatty
    // isatty は，エラーかどうかのみ見ている
    if ((int)m_args[2] == LINUX_TCGETS) {
        const int ERRNO_ENOTTY = 25;
        // 常にファイル
        // TTYである(success)と返すと，glibcがTTYのデバイス名を取得するためにreadlinkを呼び出すことがある (ttyname関数)
        if (false) {
            SetResult(true, 0);
        }
        else {
            SetResult(false, ERRNO_ENOTTY);
        }
    }
    else {
        SetResult(false, EINVAL);
    }
}

u32 Linux64SyscallConv::SeekWhenceTargetToHost(u32 flag)
{
    static u32 host_whence[] =
    {
        POSIX_SEEK_SET,
        POSIX_SEEK_CUR,
        POSIX_SEEK_END
    };
    static u32 linux_whence[] =
    {
        LINUX_SEEK_SET,
        LINUX_SEEK_CUR,
        LINUX_SEEK_END
    };
    static int whence_size = sizeof(host_whence)/sizeof(host_whence[0]);
    SyscallConstConvEnum conv(
        host_whence,
        linux_whence, 
        whence_size);

    return conv.TargetToHost(flag);
}


u32 Linux64SyscallConv::AccessModeTargetToHost(u32 flag)
{
    static u32 host_access_mode[] =
    {
        POSIX_F_OK,
        POSIX_X_OK,
        POSIX_W_OK,
        POSIX_R_OK
    };
    static u32 linux_access_mode[] =
    {
        LINUX_F_OK,
        LINUX_X_OK,
        LINUX_W_OK,
        LINUX_R_OK
    };
    static int access_mode_size = sizeof(host_access_mode)/sizeof(host_access_mode[0]);
    SyscallConstConvBitwise conv(
        host_access_mode,
        linux_access_mode, 
        access_mode_size);

    return conv.TargetToHost(flag);
}


void Linux64SyscallConv::write_stat64(u64 dest, const HostStat &src)
{
    static u32 host_st_mode[] =
    {
        POSIX_S_IFDIR,
        POSIX_S_IFCHR,
        POSIX_S_IFREG,
        POSIX_S_IFIFO,
    };
    static u32 linux64_st_mode[] =
    {
        0040000, // _S_IFDIR
        0020000, // _S_IFCHR
        0100000, // _S_IFREG
        0010000, // _S_IFIFO
    };
    static int st_mode_size = sizeof(host_st_mode)/sizeof(host_st_mode[0]);
    SyscallConstConvBitwise conv(
        host_st_mode,
        linux64_st_mode, 
        st_mode_size);

    TargetBuffer buf(GetMemorySystem(), dest, sizeof(linux64_stat64));
    linux64_stat64* t_buf = static_cast<linux64_stat64*>(buf.Get());
    memset(t_buf, 0, sizeof(linux64_stat64));

    t_buf->linux64_stdev = src.st_dev;
    t_buf->linux64_stino = src.st_ino;
    t_buf->linux64_strdev = src.st_rdev;
    t_buf->linux64_stsize = src.st_size;
    t_buf->linux64_stuid = src.st_uid;
    t_buf->linux64_stgid = src.st_gid;
    t_buf->linux64_stnlink = src.st_nlink;
    t_buf->linux64_statime = src.st_atime;
    t_buf->linux64_stmtime = src.st_mtime;
    t_buf->linux64_stctime = src.st_ctime;

    // st_blksize : 効率的にファイル・システムIO が行える"好ましい"ブロック・サイズ
    //              CentOS4 では32768
    // st_blocks  : ファイルに割り当てられた512B の数
    //              CentOS4 では8 刻みになる？

#if defined(HOST_IS_WINDOWS) || defined(HOST_IS_CYGWIN)
    static const int BLOCK_UNIT = 512*8;
    t_buf->linux64_stmode    = conv.HostToTarget(src.st_mode);
    t_buf->linux64_stblocks  = (src.st_size+BLOCK_UNIT-1)/BLOCK_UNIT*8;
    t_buf->linux64_stblksize = 32768;
#else
    t_buf->linux64_stblocks = src.st_blocks;
    t_buf->linux64_stmode = src.st_mode;
    t_buf->linux64_stblksize = src.st_blksize;
#endif

    // hostのstat(src)の要素とtargetのstatの要素のサイズが異なるかもしれないので，一旦targetのstatに代入してからエンディアンを変換する
    bool bigEndian = GetMemorySystem()->IsBigEndian();
    EndianHostToSpecifiedInPlace(t_buf->linux64_stdev, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->linux64_stino, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->linux64_strdev, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->linux64_stsize, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->linux64_stuid, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->linux64_stgid, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->linux64_stnlink, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->linux64_statime, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->linux64_stmtime, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->linux64_stctime, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->linux64_stmode, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->linux64_stblocks, bigEndian);
    EndianHostToSpecifiedInPlace(t_buf->linux64_stblksize, bigEndian);
}

