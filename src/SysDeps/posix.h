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


#ifndef __SYSDEPS_POSIX_H__
#define __SYSDEPS_POSIX_H__

#include "SysDeps/host_type.h"

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef HOST_IS_WINDOWS

#include <io.h>
#include <process.h>
#include <direct.h>

#elif defined(HOST_IS_CYGWIN)

#include <io.h>
#include <process.h>
#include <dirent.h>

#elif defined(HOST_IS_LINUX)

#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <sys/ioctl.h>
#include <sys/mount.h>

#else
#error "unknown host type"
#endif


#ifdef HOST_IS_WINDOWS
namespace Onikiri {
    namespace POSIX {
        typedef struct stat posix_struct_stat;
        const int POSIX_O_BINARY = _O_BINARY;
        const int POSIX_O_RDONLY = _O_RDONLY;
        const int POSIX_O_WRONLY = _O_WRONLY;
        const int POSIX_O_RDWR   = _O_RDWR;
        const int POSIX_O_APPEND = _O_APPEND;
        const int POSIX_O_CREAT  = _O_CREAT;
        const int POSIX_O_EXCL   = _O_EXCL;
        const int POSIX_O_TRUNC  = _O_TRUNC;

        const int POSIX_S_IFDIR  = _S_IFDIR;
        const int POSIX_S_IFCHR  = _S_IFCHR;
        const int POSIX_S_IFREG  = _S_IFREG;
        const int POSIX_S_IFIFO  = _S_IFIFO;

        const int POSIX_S_IWRITE = _S_IWRITE;
        const int POSIX_S_IREAD  = _S_IREAD;

        const int POSIX_F_OK = 0;
        const int POSIX_X_OK = 0;
        const int POSIX_W_OK = 2;
        const int POSIX_R_OK = 4;

        const int POSIX_SEEK_SET = SEEK_SET;
        const int POSIX_SEEK_CUR = SEEK_CUR;
        const int POSIX_SEEK_END = SEEK_END;

        int posix_geterrno();

        int posix_getpid();

        // Linux で確認したときに 959, 10 だったのでWindows ではその値にしておく．
        inline int posix_getuid()
            { return 959; }
        inline int posix_geteuid()
            { return 959; }
        inline int posix_getgid()
            { return 10; }
        inline int posix_getegid()
            { return 10; }

        char *posix_getcwd(char* buf, int maxlen);
        int posix_chdir(const char* path);

        int posix_open(const char* name, int flags);
        int posix_open(const char* name, int flags, int pmode);
        int posix_read(int fd, void* buf, unsigned int count);
        int posix_write(int fd, void* buf, unsigned int count);
        int posix_close(int fd);
        s64 posix_lseek(int fd, s64 offset, int whence);
        int posix_dup(int fd);

        int posix_stat(const char* path, posix_struct_stat* s);
        int posix_fstat(int fd, posix_struct_stat* s);
        int posix_lstat(const char* path, posix_struct_stat* s);

        int posix_fileno(FILE *file);
        int posix_access(const char* path, int mode);
        int posix_unlink(const char* path);
        int posix_rename(const char* oldpath, const char* newpath);

        int posix_truncate(const char* path, s64 length);
        int posix_ftruncate(int fd, s64 length);
    }
}
#elif defined(HOST_IS_CYGWIN) || defined(HOST_IS_LINUX)
namespace Onikiri {
    namespace POSIX {
        typedef struct stat posix_struct_stat;
        const int POSIX_O_BINARY = 0;
        const int POSIX_O_RDONLY = O_RDONLY;
        const int POSIX_O_WRONLY = O_WRONLY;
        const int POSIX_O_RDWR   = O_RDWR;
        const int POSIX_O_APPEND = O_APPEND;
        const int POSIX_O_CREAT  = O_CREAT;
        const int POSIX_O_EXCL   = O_EXCL;
        const int POSIX_O_TRUNC  = O_TRUNC;

        const int POSIX_S_IFDIR  = S_IFDIR;
        const int POSIX_S_IFCHR  = S_IFCHR;
        const int POSIX_S_IFREG  = S_IFREG;
        const int POSIX_S_IFIFO  = S_IFIFO;

        const int POSIX_S_IWRITE = S_IWRITE;
        const int POSIX_S_IREAD  = S_IREAD;

        const int POSIX_F_OK = F_OK;
        const int POSIX_X_OK = X_OK;
        const int POSIX_W_OK = W_OK;
        const int POSIX_R_OK = R_OK;

        const int POSIX_SEEK_SET = SEEK_SET;
        const int POSIX_SEEK_CUR = SEEK_CUR;
        const int POSIX_SEEK_END = SEEK_END;

        inline int posix_geterrno()
            { return errno; }

        inline int posix_getpid()
            { return getpid(); }

        inline int posix_getuid()
            { return getuid(); }
        inline int posix_geteuid()
            { return geteuid(); }
        inline int posix_getgid()
            { return getgid(); }
        inline int posix_getegid()
            { return getegid(); }

        inline char *posix_getcwd(char* buf, int maxlen)
            { return getcwd(buf, maxlen); }
        inline int posix_chdir(const char* path)
            { return chdir(path); }

        inline int posix_open(const char* name, int flags)
            { return open(name, flags); }
        inline int posix_open(const char* name, int flags, int pmode)
            { return open(name, flags, pmode); }
        inline int posix_read(int fd, void* buf, unsigned int count)
            { return read(fd, buf, count); }
        inline int posix_write(int fd, void* buf, unsigned int count)
            { return write(fd, buf, count); }
        inline int posix_close(int fd)
            { return close(fd); }
        inline int posix_dup(int fd)
            { return dup(fd); }

        // <TODO> 本当は，off_tのサイズ・lseek64のサポートをチェックする．
#if defined(HOST_IS_CYGWIN)
        // CYGWINはlseek64を持たない
        inline s64 posix_lseek(int fd, s64 offset, int whence)
            { return lseek(fd, offset, whence); }
#elif defined(HOST_IS_LINUX)
        inline s64 posix_lseek(int fd, s64 offset, int whence)
            { return lseek64(fd, offset, whence); }
#endif

        inline int posix_stat(const char* path, posix_struct_stat* s)
            { return stat(path, s); }
        inline int posix_fstat(int fd, posix_struct_stat* s)
            { return fstat(fd, s); }
        inline int posix_lstat(const char* path, posix_struct_stat* s)
            { return lstat(path, s); }

        inline int posix_fileno(FILE *file)
            { return fileno(file); }
        inline int posix_access(const char* path, int mode)
            { return access(path, mode); }
        inline int posix_unlink(const char* path)
            { return unlink(path); }
        inline int posix_rename(const char* oldpath, const char* newpath)
            { return rename(oldpath, newpath); }

        inline int posix_truncate(const char* path, s64 length)
            { return truncate(path, length); }
        inline int posix_ftruncate(int fd, s64 length)
            { return ftruncate(fd, length); }
    }
}
#endif

#endif
