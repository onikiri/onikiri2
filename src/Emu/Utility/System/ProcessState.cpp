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

#include "Emu/Utility/System/ProcessState.h"

#include "Env/Env.h"
#include "SysDeps/posix.h"

#include "Emu/Utility/System/VirtualPath.h"
#include "Emu/Utility/System/VirtualSystem.h"
#include "Emu/Utility/System/Syscall/SyscallConvIF.h"
#include "Emu/Utility/System/Memory/MemorySystem.h"
#include "Emu/Utility/System/Loader/LoaderIF.h"

using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::POSIX;

static string CompletePath(string relative, const string& base)
{
    if(relative.empty())
        relative = ".";

    return
        filesystem::absolute(
            filesystem::path( relative ),
            filesystem::path( base ) 
        ).string();

}


ProcessCreateParam::ProcessCreateParam(int processNumber) : 
    m_processNumber(processNumber), 
    m_stackMegaBytes(0)
{
    LoadParam();
}

ProcessCreateParam::~ProcessCreateParam()
{
    ReleaseParam();
}

const VirtualPath ProcessCreateParam::GetTargetBasePath() const
{
    ParamXMLPath xPath;
    xPath.AddArray( 
        "/Session/Emulator/Processes/Process",
        m_processNumber 
    );
    xPath.AddAttribute( "TargetBasePath" );
    String xmlFilePath;

    bool found = g_paramDB.GetSourceXMLFile( xPath, xmlFilePath );
    if( !found ){
        THROW_RUNTIME_ERROR(
            "'TargetBasePath' ('%s') is not set. 'TargetBasePath' must be set in each 'Process' node.",
            xPath.ToString().c_str()
        );
    }

    filesystem::path xmlDirPath(xmlFilePath.c_str());
    xmlDirPath.remove_filename();
    auto basePath = xmlDirPath.append(m_targetBasePath);

    VirtualPath virtualBasePath;
    virtualBasePath.SetGuestRoot("/ONIKIRI_ROOT/");
    virtualBasePath.SetHostRoot(basePath.string());
    virtualBasePath.SetVirtualPath("./");

    return virtualBasePath;
}

ProcessState::ProcessState(int pid)
    : m_pid(pid), m_threadUniqueValue(0)
{
    m_loader = 0;
    m_syscallConv = 0;
    m_controlRegs.fill(0);
}

ProcessState::~ProcessState()
{
    for_each(
        m_autoCloseFile.begin(), 
        m_autoCloseFile.end(),
        fclose 
    );

    delete m_loader;
    delete m_syscallConv;
    delete m_virtualSystem;
    delete m_memorySystem;
}

u64 ProcessState::GetEntryPoint() const
{
    return m_loader->GetEntryPoint();
}

u64 ProcessState::GetInitialRegValue(int index) const
{
    return m_loader->GetInitialRegValue(index);
}

void ProcessState::SetThreadUniqueValue(u64 value)
{
    m_threadUniqueValue = value;
}

u64 ProcessState::GetThreadUniqueValue()
{
    return m_threadUniqueValue;
}

void ProcessState::SetControlRegister(u64 index, u64 value)
{
    if (index >= MAX_CONTROL_REGISTER_NUM) {
        THROW_RUNTIME_ERROR("A control register out of range accessed.: %d", index);
    }
    m_controlRegs[(size_t)index] = value;
}

u64 ProcessState::GetControlRegister(u64 index)
{
    if (index >= MAX_CONTROL_REGISTER_NUM) {
        THROW_RUNTIME_ERROR("A control register out of range accessed.: %d", index);
    }
    return m_controlRegs.at((size_t)index);
}

void ProcessState::Init(
    const ProcessCreateParam& pcp, 
    SystemIF* simSystem,
    SyscallConvIF* syscallConv, 
    LoaderIF* loader, 
    bool bigEndian )
{
    try {
        m_memorySystem  = new MemorySystem( m_pid, bigEndian, simSystem );
        m_virtualSystem = new VirtualSystem();

        m_syscallConv = syscallConv;
        m_syscallConv->SetSystem( simSystem );
        m_loader = loader;

        const VirtualPath targetBase = pcp.GetTargetBasePath();

        VirtualPath workDir = targetBase;
        workDir.SetVirtualPath(pcp.GetTargetWorkPath());
        m_virtualSystem->SetInitialWorkingDir(workDir);

        VirtualPath cmdFileName = targetBase;
        cmdFileName.SetVirtualPath(pcp.GetCommand());
        m_loader->LoadBinary(m_memorySystem, cmdFileName.ToHost());
        m_virtualSystem->SetCommandFileName(cmdFileName);

        m_codeRange = m_loader->GetCodeRange();

        InitMemoryMap(pcp);
        InitTargetStdIO(pcp);
    }
    catch (...) {
        delete m_loader;
        m_loader = 0;
        delete m_syscallConv;
        m_syscallConv = 0;
        delete m_virtualSystem;
        m_virtualSystem = 0;
        delete m_memorySystem;
        m_memorySystem = 0;

        throw;
    }
}

// Initialize the memory map of a loaded process except loaded binary areas.
void ProcessState::InitMemoryMap(const ProcessCreateParam& pcp)
{
    // Allocate a stack area
    u64 stackMegaBytes = pcp.GetStackMegaBytes();
    if(stackMegaBytes <= 1){
        THROW_RUNTIME_ERROR("Stack size(%d MB) is too small.", stackMegaBytes);
    }
    u64 stackBytes = stackMegaBytes*1024*1024;
    
    // GetStackTail returns the end of address (e.g., 0xbfffffff), so +1
    if (m_loader->GetStackTail() < stackBytes) {
        THROW_RUNTIME_ERROR("Stack size(%d MB) is greater than the stack top address.", stackMegaBytes);
    }
    u64 stack = m_loader->GetStackTail() - stackBytes + 1;

    m_memorySystem->AssignPhysicalMemory(
        stack, stackBytes, VIRTUAL_MEMORY_ATTR_READ | VIRTUAL_MEMORY_ATTR_WRITE
    );
    //u64 stack = m_memorySystem->MMap(0, stackBytes);

    // 引数の設定
    VirtualPath targetBase = pcp.GetTargetBasePath();
    m_loader->InitArgs(
        m_memorySystem,
        stack, 
        stackBytes, 
        targetBase.CompleteInGuest(pcp.GetCommand()), // for argv, this path is virtual one
        pcp.GetCommandArguments() 
    );

    // Reserve a mmap area
    // Physical memory is allocated when mmap() is called.
    u64 initBrk = m_memorySystem->Brk(0);
    ASSERT(initBrk != 0, "Brk is not initialized");

    s64 pageSize = m_memorySystem->GetPageSize();
    initBrk = (initBrk / pageSize + 1) * pageSize;  // Align to page boundary

    // Use half of the area sandwiched between stack and brk initial values in mmap
    s64 mmapAreaSize = (stack - initBrk) / 2;
    mmapAreaSize = (mmapAreaSize / pageSize + 1) * pageSize;  // Align to page boundary
    ASSERT(mmapAreaSize > 0, "The size of mmap area is incorrect.");
    m_memorySystem->AddHeapBlock(stack - mmapAreaSize, mmapAreaSize);
}

void ProcessState::InitTargetStdIO(const ProcessCreateParam& pcp)
{


    // stdin stdout stderr の設定
    String omode[3] = {"rt", "wt", "wt"};
    int std_fd[3] = 
    {
        posix_fileno(stdin), 
        posix_fileno(stdout), 
        posix_fileno(stderr)
    };
    String std_filename[3] = 
    {
        pcp.GetStdinFilename(),
        pcp.GetStdoutFilename(), 
        pcp.GetStderrFilename()
    };

    // STDIN のファイルオープンモード
    if( pcp.GetStdinFileOpenMode() == "Text" ){
        omode[0] = "rt";
    }
    else if( pcp.GetStdinFileOpenMode() == "Binary" ){
        omode[0] = "rb";
    }
    else{
        THROW_RUNTIME_ERROR( "The file open mode of STDIN must be one of the following strings: 'Text', 'Binary'");
    }

    for (int i = 0; i < 3; i ++) {
        if (std_filename[i].empty()) {
            // 入出力として指定されたファイル名が空ならばホストの入出力を使用する
            String hostIO_Name = m_virtualSystem->GetHostIO_Name();
            m_virtualSystem->AddFDMap(std_fd[i], std_fd[i], hostIO_Name, false);
            m_virtualSystem->GetDelayUnlinker()->AddMap(std_fd[i], hostIO_Name);
        }
        else {
            VirtualPath targetBase = pcp.GetTargetBasePath();
            String targetWork = targetBase.CompleteInHost(pcp.GetTargetWorkPath());

            string filename = CompletePath(std_filename[i], targetWork );
            FILE *fp = fopen( filename.c_str(), omode[i].c_str() );
            if (fp == NULL) {
                THROW_RUNTIME_ERROR("Cannot open file '%s'.", filename.c_str());
            }
            m_autoCloseFile.push_back(fp);
            m_virtualSystem->AddFDMap(std_fd[i], posix_fileno(fp), filename, false);
            m_virtualSystem->GetDelayUnlinker()->AddMap(std_fd[i], filename);
        }
    }
}
