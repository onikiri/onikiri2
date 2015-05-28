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

#include "Sim/System/SystemManager.h"

#include "Sim/Dumper/Dumper.h"
#include "Emu/EmulatorFactory.h"

#include "Sim/Pipeline/Fetcher/Fetcher.h"
#include "Sim/Pipeline/Retirer/Retirer.h"
#include "Sim/Register/RegisterFile.h"
#include "Sim/Predictor/DepPred/RegDepPred/RegDepPredIF.h"
#include "Sim/Dependency/PhyReg/PhyReg.h"

#include "Sim/System/EmulationSystem/EmulationSystem.h"
#include "Sim/System/InorderSystem/InorderSystem.h"
#include "Sim/System/EmulationTraceSystem/EmulationTraceSystem.h"
#include "Sim/System/SimulationSystem/SimulationSystem.h"
#include "Sim/System/EmulationDebugSystem/EmulationDebugSystem.h"
#include "Sim/System/ForwardEmulator.h"

#include "Sim/Foundation/Hook/HookUtil.h"

namespace Onikiri
{
    HookPoint<SystemManager, SystemManager::ProcessNotifyParam> 
        SystemManager::s_processNotifyHook;
}

using namespace Onikiri;

SystemManager::SystemManager()
{
    m_system = NULL;
    
    m_simulationCycles = 0;
    m_simulationInsns = 0;
    m_skipInsns = 0;
}

SystemManager::~SystemManager()
{
}

void SystemManager::Initialize()
{
    LoadParam();

    m_context.resBuilder = new ResourceBuilder();

    InitializeEmulator();
    InitializeResources();
    
    GetInitialContext( &m_context.architectureStateList );

}

void SystemManager::Finalize()
{
    g_dumper.Finalize();

    m_context.emulatorWrapper.ReleaseParam();
    ReleaseParam();

    if( m_context.emulator ){
        delete m_context.emulator;
        m_context.emulator = NULL;
    }
    if( m_context.resBuilder ){
        delete m_context.resBuilder;
        m_context.resBuilder = NULL;
    }
}

void SystemManager::InitializeEmulator()
{
    g_env.PrintInternal("Emulator ... ");

    int processConfigCount = 0;
    if( !g_paramDB.Get( "/Session/Emulator/Processes/#Process", &processConfigCount, false ) ){
        THROW_RUNTIME_ERROR(
            "'/Session/Emulator/Processes/Process' node is not found. "
            "There is no process configuration. "
            "This may occur when no configuration XML file is passed. "
            "Please check your command line option."
        );
    }

    // Set the emulator
    EmulatorFactory emulatorFactory;
    m_context.emulator = emulatorFactory.Create( m_context.targetArchitecture, this );
    m_context.emulator->SetExtraOpDecoder( &m_extraOpDecoder );
    SimISAInfo::TestISAInfo( m_context.emulator->GetISAInfo() );

    g_env.PrintInternal("initialized.\n");
}

void SystemManager::InitializeResources()
{
    g_env.PrintInternal("Resources ... ");

    // Set the external emulator to the resource builder before constructing and connecting.
    PhysicalResourceNodeInfo info;
    info.name       = "emulator";
    info.typeName   = "EmulatorIF";
    info.paramPath  = "/Session/Emulator/";
    info.resultPath = "/Session/Result/Resource/Emulator/";
    info.resultRootPath = "/Session/Result/Resource/Emulator/";
    m_context.emulatorWrapper.SetInfo( info );
    m_context.emulatorWrapper.SetEmulator( m_context.emulator ); 

    ResourceBuilder::ResNode node;
    node.count = 1;
    node.external = true;
    node.instances.push_back( &m_context.emulatorWrapper );
    node.name = info.name;
    node.type = info.typeName;
    node.paramPath = info.paramPath;
    node.resultPath = info.resultPath;
    m_context.resBuilder->SetExternalResource( &node );

    // Construct objects
    m_context.resBuilder->Construct();

    m_context.resBuilder->GetResourceByType( "Core",   m_context.cores );
    m_context.resBuilder->GetResourceByType( "Thread", m_context.threads );
    m_context.resBuilder->GetResourceByType( "Cache",  m_context.caches );
    m_context.resBuilder->GetResourceByType( "GlobalClock", m_context.globalClock );
    m_context.resBuilder->GetResourceByType( "ForwardEmulator", m_context.forwardEmulator );

    InitializeSimulationContext();

    g_dumper.Initialize( m_context.cores, m_context.threads );

    g_env.PrintInternal("initialized.\n");
}

void SystemManager::InitializeSimulationContext()
{
    int processCount = m_context.emulator->GetProcessCount();
    if( processCount > m_context.threads.GetSize() ){
        THROW_RUNTIME_ERROR(
            "The number of the processes is greater than the number of the threads." 
        );
    }

    // Get initial contexts
    ArchitectureStateList archStateList;
    GetInitialContext( &archStateList );

    // Set the initial contexts
    SetSimulationContext( archStateList );
}

void SystemManager::GetInitialContext( ArchitectureStateList* archStateList )
{

    int processCount = m_context.emulator->GetProcessCount();
    archStateList->resize( processCount );
    for( int pid = 0; pid < processCount; pid++ ){
        // PC
        PC pc = m_context.emulator->GetEntryPoint(pid);
        pc.tid = pid;   // The thread id from the GetEntryPoint() is not set in the emulator
        (*archStateList)[pid].pc = pc;
    }

    ISAInfoIF* isaInfo = m_context.emulator->GetISAInfo();

    // 論理レジスタの初期値を emulator に教えてもらってセット
    int logicalRegCount = isaInfo->GetRegisterCount();
    for( int pid = 0; pid < processCount; pid++ ){
        (*archStateList)[pid].registerValue.resize( logicalRegCount );
        for (int i = 0; i < logicalRegCount; i ++) {
            (*archStateList)[pid].registerValue[i] = m_context.emulator->GetInitialRegValue(pid, i);
        }
    }
}

bool SystemManager::SetSimulationContext( const ArchitectureStateList& archStateList )
{
    ISAInfoIF* isaInfo  = m_context.emulator->GetISAInfo();
    int processCount    = m_context.emulator->GetProcessCount();
    int logicalRegCount = isaInfo->GetRegisterCount();

    for( int i = 0; i < m_context.cores.GetSize(); i++ ){
        Core* core = m_context.cores[i];

        // fetch index
        core->GetFetcher()->SetInitialNumFetchedOp(0);
        // retire count
        core->GetRetirer()->SetInitialNumRetiredOp(0, 0, m_context.executionInsns );

        // register readiness
        RegisterFile* regFile = core->GetRegisterFile();
        for(int i = 0; i < regFile->GetTotalCapacity(); ++i) {
            PhyReg* reg = regFile->GetPhyReg(i);
            reg->Set();
        }
    }

    bool remainingProcess = false;
    for( int pid = 0; pid < processCount; pid++ ){
        const ArchitectureState& context = archStateList[pid];
        if( context.pc.address != 0 ){
            remainingProcess = true;

            // fetch pc
            PC pc = context.pc;
            pc.tid = pid;   // A thread id is not set in the emulator
            m_context.threads[pid]->InitializeContext( pc );
            m_context.threads[pid]->Activate( true );

            RegDepPredIF* regDepPred = m_context.threads[pid]->GetRegDepPred();
            RegisterFile* regFile    = m_context.threads[pid]->GetCore()->GetRegisterFile();

            // register value
            for(int i = 0; i < logicalRegCount; ++i) {
                int phyRegNo = regDepPred->PeekReg(i);
                PhyReg* reg  = regFile->GetPhyReg( phyRegNo );
                reg->SetVal( context.registerValue[i] );
            }
        }
        else{
            // Deactivate the process
            m_context.threads[pid]->Activate( false );
        }
    }

    // Threads that exceed the number of the processes are deactivated.
    for( int i = processCount; i < m_context.threads.GetSize(); i++ ){
        PC entryPoint( PID_INVALID, i, 0 );
        m_context.threads[i]->InitializeContext(    entryPoint );
        m_context.threads[i]->Activate( false );
    }

    m_context.forwardEmulator->SetContext( archStateList );
    return remainingProcess;
}

void SystemManager::RunSimulation( SystemContext* context )
{
    NotifyChangingMode( PhysicalResourceNode::SM_SIMULATION );
    SimulationSystem simulationSystem;
    simulationSystem.Run( context );
}

void SystemManager::RunEmulation( SystemContext* context )
{
    NotifyChangingMode( PhysicalResourceNode::SM_EMULATION );
    EmulationSystem emulationSystem;
    emulationSystem.Run( context );
}

void SystemManager::RunEmulationTrace( SystemContext* context )
{
    NotifyChangingMode( PhysicalResourceNode::SM_EMULATION );
    EmulationTraceSystem emulationTraceSystem;
    emulationTraceSystem.Run( context );
}

void SystemManager::RunEmulationDebug( SystemContext* context )
{
    EmulationDebugSystem emulationDebugSystem;
    emulationDebugSystem.Run( context );
}

void SystemManager::RunInorder( SystemContext* context )
{
    NotifyChangingMode( PhysicalResourceNode::SM_INORDER );
    InorderSystem inorderSystem;
    inorderSystem.Run( context );
}

void SystemManager::SetSystem( SystemIF* system )
{
    m_system = system;
}

void SystemManager::Main()
{
    bool initialized = false;
    bool thrown = false;
    std::string errMessage;

    // Initialize the system.
    try{
        Initialize();
        initialized = true;
    }
    catch( const std::exception& error ){
        errMessage = error.what();
        thrown = true;
    }

    // Run
    try{
        if( initialized ){
            Run();
        }
    }
    catch( const std::exception& error ){
        if( !thrown ){
            errMessage = error.what();
        }
        thrown = true;
    }

    // Finalize the system.
    try{
        Finalize();
    }
    catch( const std::exception& error ){
        if( !thrown ){
            errMessage = error.what();
        }
        thrown = true;
    }

    if( thrown ){
        throw std::runtime_error( errMessage.c_str() );
    }
}

void SystemManager::Run()
{
    const String& mode = m_context.mode;

    m_context.executionInsns  = m_simulationInsns;
    m_context.executionCycles = m_simulationCycles;

    if( mode == "Emulation" ){
        if( m_context.executionInsns == 0 ){
            RUNTIME_WARNING( "'@SimulationInsns' is 0 and nothing is executed in 'Emulation' mode." );
        }
        RunEmulation( &m_context );
    }
    else if( mode == "CreateEmulationTrace" ){
        if( m_context.executionInsns == 0 ){
            RUNTIME_WARNING( "'@SimulationInsns' is 0 and nothing is executed in 'CreateEmulationTrace' mode." );
        }
        RunEmulationTrace( &m_context );
    }
    else if( mode == "EmulationWithDebug" ){
        if( m_context.executionInsns == 0 ){
            RUNTIME_WARNING( "'@SimulationInsns' is 0 and nothing is executed in 'CreateEmulationTrace' mode." );
        }
        RunEmulationDebug( &m_context );
    }
    else if( mode == "Inorder" ) {
        // Run emulation
        m_context.executionInsns  = m_skipInsns;
        m_context.executionCycles = 0;
        RunEmulation( &m_context );
        m_skippedInsns = m_context.executedInsns;

        // Run inorder
        m_context.executionInsns  = m_simulationInsns;
        m_context.executionCycles = m_simulationCycles;
        m_context.executedInsns.clear();
        m_context.executedCycles = 0;
        if( SetSimulationContext( m_context.architectureStateList ) ){
            RunInorder( &m_context );
        }
    }
    else if( mode == "SkipByInorder" ) {
        // Run inorder
        m_context.executionInsns  = m_skipInsns;
        m_context.executionCycles = 0;
        RunInorder( &m_context );
        m_skippedInsns = m_context.executedInsns;

        // Run simulation
        m_context.executionInsns  = m_simulationInsns;
        m_context.executionCycles = m_simulationCycles;
        m_context.executedInsns.clear();
        m_context.executedCycles = 0;
        if( SetSimulationContext( m_context.architectureStateList ) ){
            RunSimulation( &m_context );
        }
    }
    else if( mode == "Simulation" ){

        // Run emulation
        m_context.executionInsns  = m_skipInsns;
        m_context.executionCycles = 0;
        RunEmulation( &m_context );
        m_skippedInsns = m_context.executedInsns;

        // Run simulation
        m_context.executionInsns  = m_simulationInsns;
        m_context.executionCycles = m_simulationCycles;
        m_context.executedInsns.clear();
        m_context.executedCycles = 0;
        if( SetSimulationContext( m_context.architectureStateList ) ){
            RunSimulation( &m_context );
        }
    }
    else{
        THROW_RUNTIME_ERROR(
            "An unknown simulation mode is specified in the"
            "'/Session/System/@Mode'\n"
            "This parameter must be one of the following strings : \n"
            "[Emulation, Simulation, Inorder, CreateEmulationTrace, SkipByInorder]"
        );
    }

    m_executedInsns  = m_context.executedInsns;
    m_executedCycles = m_context.executedCycles;
    m_ipc.clear();
    for( size_t i = 0; i < m_executedInsns.size(); ++i ){
        if( m_executedCycles != 0 ){
            m_ipc.push_back( (double)m_executedInsns[i] / (double)m_executedCycles );
        }
        else{
            m_ipc.push_back( 1.0 );
        }
    }

}

// SystemIF
 
void SystemManager::NotifyProcessTermination(int pid)
{   
    ProcessNotifyParam param = { PNT_TERMINATION, pid, Addr(), 0/*size*/, 0/*totalSize*/ };
    HookEntry(
        this,
        &SystemManager::NotifyProcessTerminationBody,
        &s_processNotifyHook,
        &param 
    );
}

void SystemManager::NotifySyscallReadFileToMemory(const Addr& addr, u64 size)
{
    ProcessNotifyParam param = 
    {
        PNT_READ_FILE_TO_MEMORY, addr.pid, addr, size, 0 
    };
    HookEntry(
        this,
        &SystemManager::NotifySyscallReadFileToMemoryBody,
        &s_processNotifyHook,
        &param 
    );
}

void SystemManager::NotifySyscallWriteFileFromMemory(const Addr& addr, u64 size)
{
    ProcessNotifyParam param = 
    {
        PNT_WRITE_FILE_FROM_MEMORY, addr.pid, addr, size, 0
    };
    HookEntry(
        this,
        &SystemManager::NotifySyscallWriteFileFromMemoryBody,
        &s_processNotifyHook,
        &param 
    );
}

void SystemManager::NotifyMemoryAllocation(const Addr& addr, u64 size, bool allocate)
{
    // Update process memory usage
    if( m_processMemoryUsage.size() <= (size_t)addr.pid ){
        m_processMemoryUsage.resize( addr.pid + 1 );
    }
    if( allocate ){
        m_processMemoryUsage[addr.pid] += size;
    }
    else{
        m_processMemoryUsage[addr.pid] -= size;
    }

    ProcessNotifyParam param = 
    {
        allocate ? PNT_ALLOCATE_MEMORY : PNT_FREE_MEMORY, 
        addr.pid, 
        addr, 
        size,
        m_processMemoryUsage[addr.pid]
    };
    HookEntry(
        this,
        &SystemManager::NotifyMemoryAllocationBody,
        &s_processNotifyHook,
        &param 
    );
}

//
// The bodies of notify methods
//
void SystemManager::NotifyProcessTerminationBody( ProcessNotifyParam* param )
{
    if(!m_system)
        return;

    m_system->NotifyProcessTermination( param->pid );
}

void SystemManager::NotifySyscallReadFileToMemoryBody( ProcessNotifyParam* param )
{
    if(!m_system)
        return;

    m_system->NotifySyscallReadFileToMemory( param->addr, param->size );
}

void SystemManager::NotifySyscallWriteFileFromMemoryBody( ProcessNotifyParam* param )
{
    if(!m_system)
        return;

    m_system->NotifySyscallWriteFileFromMemory( param->addr, param->size );
}

void SystemManager::NotifyMemoryAllocationBody( ProcessNotifyParam* param )
{
    if(!m_system)
        return;

    m_system->NotifyMemoryAllocation( 
        param->addr,
        param->size, 
        param->type == PNT_ALLOCATE_MEMORY ? true : false 
    );
}

void SystemManager::NotifyChangingMode( PhysicalResourceNode::SimulationMode mode )
{
    ASSERT( m_context.resBuilder != NULL );
    std::vector<PhysicalResourceNode*> res =
        m_context.resBuilder->GetAllResources();
    for( size_t i = 0; i < res.size(); i++ ){
        res[i]->ChangeSimulationMode( mode );
    }
}

