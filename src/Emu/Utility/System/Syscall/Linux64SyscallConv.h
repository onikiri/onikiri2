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


#ifndef __EMULATORUTILITY_LINUX64SYSCALLCONV_H__
#define __EMULATORUTILITY_LINUX64SYSCALLCONV_H__

#include "Emu/Utility/System/Syscall/SyscallConvIF.h"
#include "Emu/Utility/System/VirtualSystem.h"
#include "Emu/Utility/System/ProcessState.h"

namespace Onikiri {
    class SystemIF;

    namespace EmulatorUtility {
        class MemorySystem;
        class OpEmulationState;

        class Linux64SyscallConv : public EmulatorUtility::SyscallConvIF
        {
        private:
            Linux64SyscallConv() {}
        public:
            Linux64SyscallConv(EmulatorUtility::ProcessState* processState);
            virtual ~Linux64SyscallConv();

            // システムコールの引数 (index番目) を設定する
            virtual void SetArg(int index, u64 value);

            // SetArg によって与えられた引数に従ってシステムコールを行う
            virtual void Execute(EmulatorUtility::OpEmulationState* opState) = 0;

            // Exec した結果を得る
            virtual u64 GetResult(int index);
            virtual void SetSystem(SystemIF* system);

        private:
            static const int MaxResultCount = 2;

            std::vector<u64> m_args;
            std::vector<u64> m_results;
            EmulatorUtility::ProcessState* m_processState;
            SystemIF* m_simulatorSystem;


        protected:
            static const int MaxArgCount = 16;

            u64 GetArg(int index) const
            {
                return m_args[index];
            }

            EmulatorUtility::ProcessState* GetProcessState()
            {
                return m_processState;
            }

            EmulatorUtility::MemorySystem* GetMemorySystem()
            {
                return m_processState->GetMemorySystem();
            }

            EmulatorUtility::VirtualSystem* GetVirtualSystem()
            {
                return m_processState->GetVirtualSystem();
            }

            void SetResult(bool success, u64 result);

            virtual void syscall_exit(EmulatorUtility::OpEmulationState* opState);

            virtual void syscall_open(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_close(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_read(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_write(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_readv(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_writev(EmulatorUtility::OpEmulationState* opState);

            virtual void syscall_lseek(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_unlink(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_rename(EmulatorUtility::OpEmulationState* opState);

            virtual void syscall_mmap(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_munmap(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_mremap(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_mprotect(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_fcntl(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_uname(EmulatorUtility::OpEmulationState* opState);

            virtual void syscall_brk(EmulatorUtility::OpEmulationState* opState);

            virtual void syscall_getpid(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_getuid(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_geteuid(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_getgid(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_getegid(EmulatorUtility::OpEmulationState* opState);

            //virtual void syscall_setuid(EmulatorUtility::OpEmulationState* opState);
            //virtual void syscall_seteuid(EmulatorUtility::OpEmulationState* opState);
            //virtual void syscall_setgid(EmulatorUtility::OpEmulationState* opState);
            //virtual void syscall_setegid(EmulatorUtility::OpEmulationState* opState);
            //virtual void syscall_setreuid(EmulatorUtility::OpEmulationState* opState);
            //virtual void syscall_setregid(EmulatorUtility::OpEmulationState* opState);

            virtual void syscall_access(EmulatorUtility::OpEmulationState* opState);
        //  virtual void syscall_fstat(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_stat64(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_lstat64(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_fstat64(EmulatorUtility::OpEmulationState* opState);

            virtual void syscall_ioctl(EmulatorUtility::OpEmulationState* opState);
//          virtual void syscall_readlink(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_mkdir(EmulatorUtility::OpEmulationState* opState);

            virtual void syscall_dup(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_truncate(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_ftruncate(EmulatorUtility::OpEmulationState* opState);

            virtual void syscall_kill(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_tgkill(EmulatorUtility::OpEmulationState* opState);

            void kill_helper(EmulatorUtility::OpEmulationState* opState, int pid, int sig);

            virtual void syscall_ignore(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_time(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_times(EmulatorUtility::OpEmulationState* opState);
            virtual void syscall_gettimeofday(EmulatorUtility::OpEmulationState* opState);

            virtual void syscall_getcwd(EmulatorUtility::OpEmulationState* opState);

            // arch dependent

            // concversion
            virtual void write_stat64(u64 dest, const EmulatorUtility::HostStat &src);
            virtual int Get_MAP_ANONYMOUS() = 0;
            virtual int Get_MREMAP_MAYMOVE() = 0;
            virtual int Get_CLK_TCK() = 0;

            virtual u32 OpenFlagTargetToHost(u32 flag) = 0;
            virtual u32 SeekWhenceTargetToHost(u32 flag);
            virtual u32 AccessModeTargetToHost(u32 flag);
        };

    } // namespace AlphaLinux
} // namespace Onikiri

#endif
