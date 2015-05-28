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


#ifndef SIM_SYSTEM_SYSTEM_MANAGER_H
#define SIM_SYSTEM_SYSTEM_MANAGER_H

#include "Sim/System/SystemBase.h"
#include "Sim/System/ExtraOpDecoder.h"

namespace Onikiri
{
    class SystemManager : 
        public SystemManagerIF, SystemIF,
        public ParamExchange
    {
    public:
        // Hooks for the notify of process information
        enum PROCESS_NOTIFY_TYPE
        {
            PNT_TERMINATION = 0,        // process termination
            PNT_READ_FILE_TO_MEMORY,    // read data from a file to memory
            PNT_WRITE_FILE_FROM_MEMORY, // write data from memory to a file
            PNT_ALLOCATE_MEMORY,        // allocate memory
            PNT_FREE_MEMORY,            // free memory
        };
        
        struct ProcessNotifyParam
        {
            PROCESS_NOTIFY_TYPE type;
            int  pid;
            Addr addr;
            u64  size;
            u64  totalSize;
        };
        static HookPoint<SystemManager, ProcessNotifyParam> s_processNotifyHook;

        SystemManager();
        virtual ~SystemManager();

        virtual void Main();

        // SystemIF
        virtual void NotifyProcessTermination(int pid);
        virtual void NotifySyscallReadFileToMemory(const Addr& addr, u64 size);
        virtual void NotifySyscallWriteFileFromMemory(const Addr& addr, u64 size);
        virtual void NotifyMemoryAllocation(const Addr& addr, u64 size, bool allocate);
        BEGIN_PARAM_MAP( "/Session/" )
            BEGIN_PARAM_PATH("Emulator/")
                PARAM_ENTRY("@TargetArchitecture",      m_context.targetArchitecture)
            END_PARAM_PATH()
            BEGIN_PARAM_PATH("Simulator/")
                PARAM_ENTRY("System/@Mode",             m_context.mode)
                PARAM_ENTRY("System/@SimulationCycles", m_simulationCycles)
                PARAM_ENTRY("System/@SimulationInsns",  m_simulationInsns)
                PARAM_ENTRY("System/@SkipInsns",        m_skipInsns)
                PARAM_ENTRY("System/Inorder/@EnableBPred",      m_context.inorderParam.enableBPred  )
                PARAM_ENTRY("System/Inorder/@EnableHMPred", m_context.inorderParam.enableHMPred )
                PARAM_ENTRY("System/Inorder/@EnableCache",      m_context.inorderParam.enableCache  )
                PARAM_ENTRY("System/Debug/@DebugPort",  m_context.debugParam.debugPort  )
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( "Result/" )
                PARAM_ENTRY("System/@ExecutedCycles",   m_executedCycles)
                PARAM_ENTRY("System/@ExecutedInsns",    m_executedInsns)
                PARAM_ENTRY("System/@SkippedInsns",     m_skippedInsns)
                PARAM_ENTRY("System/@IPC",              m_ipc)
                PARAM_ENTRY("System/@ProcessMemoryUsage",   m_processMemoryUsage)
            END_PARAM_PATH()
        END_PARAM_MAP()


    protected:
        typedef SystemBase::SystemContext SystemContext;

        SystemContext m_context;
        SystemIF*     m_system;

        s64 m_simulationCycles; // simulation を行うサイクル数
        s64 m_executedCycles;   // 実際に実行されたサイクル数
        
        s64 m_simulationInsns;  // simulation を行う命令数
        s64 m_skipInsns;        // はじめにskipする命令数

        std::vector<s64> m_executedInsns;   // 実際に実行された命令数
        std::vector<s64> m_skippedInsns;    // 実際にスキップ実行されたサイクル数

        std::vector<double> m_ipc;          // ipc
        std::vector<u64> m_processMemoryUsage;  // プロセス毎のメモリ使用量
        ExtraOpDecoder m_extraOpDecoder;

        virtual void InitializeEmulator();
        virtual void InitializeResources();
        virtual void InitializeSimulationContext();

        virtual void Initialize();
        virtual void Finalize();
        virtual void Run();

        virtual void GetInitialContext( ArchitectureStateList* archState );
        virtual bool SetSimulationContext( const ArchitectureStateList& archState );

        virtual void RunSimulation( SystemContext* context );
        virtual void RunEmulation( SystemContext* context );
        virtual void RunEmulationTrace( SystemContext* context );
        virtual void RunEmulationDebug( SystemContext* context );
        virtual void RunInorder( SystemContext* context );

        virtual void NotifyProcessTerminationBody( ProcessNotifyParam* );
        virtual void NotifySyscallReadFileToMemoryBody( ProcessNotifyParam* );
        virtual void NotifySyscallWriteFileFromMemoryBody( ProcessNotifyParam* );
        virtual void NotifyMemoryAllocationBody( ProcessNotifyParam* );

        virtual void SetSystem( SystemIF* system );
        
        virtual void NotifyChangingMode( PhysicalResourceNode::SimulationMode mode );

    };
}   // namespace Onikiri

#endif // #ifndef SIM_SYSTEM_SYSTEM_MANAGER_H

