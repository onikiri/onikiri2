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
#if defined(HOST_IS_WINDOWS)
#include <windows.h>
#elif defined(HOST_IS_CYGWIN) || defined(HOST_IS_LINUX)
//
#endif
#include "SysDeps/posix.h"

using namespace std;
using namespace Onikiri;
using namespace Onikiri::POSIX;

#if defined(HOST_IS_WINDOWS)
//
// Windows
//
namespace {
    // 明らかにスレッドセーフではないがどうせシングルスレッドなので
    // 万一マルチスレッドにするようなことがあればTLSでも使って適切に
    int posix_errno = 0;

    // MSDNによると errno の番号はUNIXと同じ
    // x86_64 LinuxとERANGE=34まで同じことを確認

    // エラーをチェックして必要ならerrnoを取得する
    template <typename T, T ErrorVal>
    T check_error(T result)
    {
        if (result == ErrorVal) {
            posix_errno = errno;
        }
        return result;
    }
}

namespace Onikiri {
    namespace POSIX {
        int posix_geterrno()
        {
            return posix_errno;
        }

        int posix_getpid()
        {
            return _getpid();
        }
        char *posix_getcwd(char* buf, int maxlen)
        {
            return check_error<char*, 0>( _getcwd(buf, maxlen) );
        }
        int posix_chdir(const char* path)
        {
            return check_error<int, -1>( _chdir(path) );
        }

        int posix_open(const char* name, int flags)
        {
            return check_error<int, -1>( _open(name, flags | POSIX_O_BINARY) );
        }
        int posix_open(const char* name, int flags, int pmode)
        {
            return check_error<int, -1>( _open(name, flags | POSIX_O_BINARY, pmode) );
        }
        int posix_read(int fd, void* buf, unsigned int count)
        {
            return check_error<int, -1>( _read(fd, buf, count) );
        }
        int posix_write(int fd, void* buf, unsigned int count)
        {
            return check_error<int, -1>( _write(fd, buf, count) );
        }
        int posix_close(int fd)
        {
            return check_error<int, -1>( _close(fd) );
        }
        s64 posix_lseek(int fd, s64 offset, int whence)
        {
            return check_error<s64, -1>( _lseeki64(fd, offset, whence) );
        }
        int posix_dup(int fd)
        {
            return check_error<int, -1>( _dup(fd) );
        }


        int posix_fstat(int fd, posix_struct_stat* s)
        {
            return check_error<int, -1>( fstat(fd, s) );
        }
        int posix_stat(const char* path, posix_struct_stat* s)
        {
            return check_error<int, -1>( stat(path, s) );
        }
        int posix_lstat(const char* path, posix_struct_stat* s)
        {
            // Windows にはシンボリックリンクがないので，statとlstatは同じ
            return posix_stat(path, s);
        }

        int posix_fileno(FILE *file)
        {
            return check_error<int, -1>( _fileno(file) );
        }
        int posix_access(const char* path, int mode)
        {
            return check_error<int, -1>( _access(path, mode) );
        }
        int posix_unlink(const char* path)
        {
            return check_error<int, -1>( _unlink(path) );
        }
        int posix_rename(const char* oldpath, const char* newpath)
        {
            if (MoveFileEx(oldpath, newpath, MOVEFILE_REPLACE_EXISTING)) {
                return 0;
            }
            else {
                switch (GetLastError()) {
                case ERROR_FILE_NOT_FOUND:
                case ERROR_PATH_NOT_FOUND:
                    posix_errno = ENOENT;
                    break;
                case ERROR_ACCESS_DENIED:
                default:
                    posix_errno = EACCES;
                    break;
                }
                return -1;
            }
        }

        int posix_truncate(const char* path, s64 length)
        {
            int fd = posix_open(path, POSIX_O_RDWR);
            if (fd == -1)
                return -1;
            int result = posix_ftruncate(fd, length);
            posix_close(fd);
            return result;
        }

        int posix_ftruncate(int fd, s64 length)
        {
            int result = _chsize_s(fd, length);
            if (result != 0) {
                posix_errno = result;
                return -1;
            }
            else {
                return 0;
            }
        }
    }
}

#elif defined(HOST_IS_CYGWIN) || defined(HOST_IS_LINUX)
//
// Unix
//
#endif

