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


#ifndef __EMULATORUTILITY_PROCESSINFO_H__
#define __EMULATORUTILITY_PROCESSINFO_H__

#include <boost/pool/pool.hpp>

#include "Interface/EmulatorIF.h"
#include "Interface/OpStateIF.h"
#include "Env/Param/ParamExchange.h"

namespace Onikiri {
    class SystemIF;

    namespace EmulatorUtility {

        class VirtualSystem;
        class MemorySystem;
        class LoaderIF;
        class SyscallConvIF;

        // Emulator/Processes/Process を読み出すクラス
        class ProcessCreateParam : public ParamExchange
        {
        public:
            explicit ProcessCreateParam(int processNumber);
            ~ProcessCreateParam();

            const String  GetTargetBasePath() const;
            const String& GetTargetWorkPath() const { return m_targetWorkPath; }
            const String& GetCommand() const { return m_command; }
            const String& GetCommandArguments() const { return m_commandArguments; }
            const String& GetStdinFilename() const { return m_stdinFilename; }
            const String& GetStdinFileOpenMode() const { return m_stdinFileOpenMode; }
            const String& GetStdoutFilename() const { return m_stdoutFilename; }
            const String& GetStderrFilename() const { return m_stderrFilename; }
            const u64 GetStackMegaBytes() const { return m_stackMegaBytes; }
        private:
            // param_map
            int m_processNumber;
            String m_targetBasePath;
            String m_targetWorkPath;
            String m_command;
            String m_commandArguments;
            String m_stdinFilename;
            String m_stdinFileOpenMode;
            String m_stdoutFilename;
            String m_stderrFilename;
            u64 m_stackMegaBytes;

        public:
            BEGIN_PARAM_MAP_INDEX("/Session/Emulator/Processes/Process", m_processNumber)
                PARAM_ENTRY("@TargetBasePath", m_targetBasePath)
                PARAM_ENTRY("@TargetWorkPath", m_targetWorkPath)
                PARAM_ENTRY("@Command", m_command)
                PARAM_ENTRY("@CommandArguments", m_commandArguments)
                PARAM_ENTRY("@STDIN", m_stdinFilename)
                PARAM_ENTRY("@STDINFileOpenMode", m_stdinFileOpenMode)
                PARAM_ENTRY("@STDOUT", m_stdoutFilename)
                PARAM_ENTRY("@STDERR", m_stderrFilename)
                PARAM_ENTRY("@StackMegaBytes", m_stackMegaBytes)
            END_PARAM_MAP()
        };

        // プロセスの状態 (初期状態・アドレス空間・物理メモリ・システムコール) を保持する
        class ProcessState
        {
        public:
            ProcessState(int pid);
            ~ProcessState();

            template<class Traits>
            void Init(const ProcessCreateParam& createParam, SystemIF* simSystem) 
            {
                Init(
                    createParam, 
                    simSystem, 
                    new typename Traits::SyscallConvType(this), 
                    new typename Traits::LoaderType(), 
                    Traits::IsBigEndian
                );
            }

            // エントリーポイントを得る
            u64 GetEntryPoint() const;
            // レジスタの初期値を得る
            u64 GetInitialRegValue(int index) const;

            int GetPID() const { return m_pid; }

            // コード領域の範囲を返す (開始アドレス，バイト数) (Emulatorの最適化用)
            std::pair<u64, size_t> GetCodeRange() const { return m_codeRange; }

            SyscallConvIF* GetSyscallConv() { return m_syscallConv; }
            MemorySystem* GetMemorySystem() { return m_memorySystem; }
            VirtualSystem* GetVirtualSystem() { return m_virtualSystem; }

            // スレッド固有値
            void SetThreadUniqueValue(u64 value);
            u64 GetThreadUniqueValue();

        private:
            void Init(const ProcessCreateParam& pcp, SystemIF* simSystem, SyscallConvIF* syscallConv, LoaderIF* loader, bool bigEndian);
            void InitStack(const ProcessCreateParam& createParam);
            void InitTargetStdIO(const ProcessCreateParam& createParam);

            int m_pid;
            std::pair<u64, size_t> m_codeRange;
            VirtualSystem* m_virtualSystem;
            MemorySystem* m_memorySystem;
            LoaderIF* m_loader;
            SyscallConvIF* m_syscallConv;

            // デストラクト時に自動で閉じるファイル
            std::vector<FILE*> m_autoCloseFile;
            
            // rduniq, wruniq でアクセスするスレッド固有の値
            u64 m_threadUniqueValue;
        };

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif
