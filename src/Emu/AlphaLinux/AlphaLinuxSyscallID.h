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


#ifndef __ALPHALINUX_SYSCALLID_H__
#define __ALPHALINUX_SYSCALLID_H__

namespace Onikiri {
    namespace AlphaLinux {

        namespace SyscallID {
            const int syscall_id_osf_syscall = 0;
            const int syscall_id_exit = 1;
            const int syscall_id_fork = 2;
            const int syscall_id_read = 3;
            const int syscall_id_write = 4;

            const int syscall_id_close = 6;

            const int syscall_id_link = 9;
            const int syscall_id_unlink = 10;

            const int syscall_id_chdir = 12;
            const int syscall_id_fchdir = 13;

            const int syscall_id_chmod = 15;
            const int syscall_id_chown = 16;
            const int syscall_id_brk = 17;

            const int syscall_id_lseek = 19;
            const int syscall_id_getxpid = 20;

            const int syscall_id_setuid = 23;
            const int syscall_id_getxuid = 24;

            const int syscall_id_access = 33;

            const int syscall_id_sync = 36;
            const int syscall_id_kill = 37;

            const int syscall_id_setpgid = 39;

            const int syscall_id_dup = 41;

            const int syscall_id_open = 45;

            const int syscall_id_getxgid = 47;
            const int syscall_id_osf_sigprocmask = 48;

            const int syscall_id_ioctl = 54;

            const int syscall_id_symlink = 57;
            const int syscall_id_readlink = 58;
            const int syscall_id_execve = 59;
            const int syscall_id_umask = 60;

            const int syscall_id_vfork = 66;
            const int syscall_id_stat = 67;
            const int syscall_id_lstat = 68;

            const int syscall_id_mmap = 71;
            const int syscall_id_munmap = 73;
            const int syscall_id_mprotect = 74;

            const int syscall_id_getgroups = 79;
            const int syscall_id_setgroups = 80;

            const int syscall_id_fstat = 91;
            const int syscall_id_fcntl = 92;
            const int syscall_id_osf_select = 93;
            const int syscall_id_osf_gettimeofday = 116;
            const int syscall_id_osf_getrusage = 117;

            const int syscall_id_readv = 120;
            const int syscall_id_writev = 121;
            const int syscall_id_osf_settimeofday = 122;

            const int syscall_id_fchown = 123;
            const int syscall_id_fchmod = 124;
            const int syscall_id_setreuid = 126;
            const int syscall_id_setregid = 127;

            const int syscall_id_rename = 128;
            const int syscall_id_truncate = 129;
            const int syscall_id_ftruncate = 130;
            const int syscall_id_flock = 131;

            const int syscall_id_mkdir = 136;
            const int syscall_id_rmdir = 137;

            const int syscall_id_getrlimit = 144;
            const int syscall_id_setrlimit = 145;

            const int syscall_id_osf_pid_block = 153;
            const int syscall_id_osf_pid_unblock = 154;

            const int syscall_id_sigaction = 156;

            const int syscall_id_getpgid = 233;
            const int syscall_id_getsid = 234;

            const int syscall_id_osf_getsysinfo = 256;
            const int syscall_id_osf_setsysinfo = 257;

            const int syscall_id_times = 323;

            const int syscall_id_uname = 339;
            const int syscall_id_nanosleep = 340;
            const int syscall_id_mremap = 341;

            const int syscall_id_setresuid = 343;
            const int syscall_id_getresuid = 344;

            const int syscall_id_rt_sigaction = 352;
            const int syscall_id_rt_sigprocmask = 353;

            const int syscall_id_select = 358;
            const int syscall_id_gettimeofday = 359;
            const int syscall_id_settimeofday = 360;

            const int syscall_id_utimes = 363;
            const int syscall_id_getrusage = 364;

            const int syscall_id_getcwd = 367;

            const int syscall_id_gettid = 378;

            const int syscall_id_exit_group = 405;

            const int syscall_id_tgkill = 424;

            const int syscall_id_stat64 = 425;
            const int syscall_id_lstat64 = 426;
            const int syscall_id_fstat64 = 427;

        } // namespace SyscallID

        using namespace SyscallID;

    }   // namespace AlphaLinux
} // namespace Onikiri

#endif
