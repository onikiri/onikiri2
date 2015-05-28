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

#include "Emu/Utility/System/ProcessState.h"

#include "Env/Env.h"
#include "SysDeps/posix.h"

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

const String ProcessCreateParam::GetTargetBasePath() const
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

    filesystem::path xmlDirPath( xmlFilePath.c_str() );
    xmlDirPath.remove_filename();

    return CompletePath( 
        m_targetBasePath, 
        xmlDirPath.string()
    );
}

ProcessState::ProcessState(int pid)
    : m_pid(pid), m_threadUniqueValue(0)
{
    m_loader = 0;
    m_syscallConv = 0;
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

        string targetBase = pcp.GetTargetBasePath();

        m_virtualSystem->SetInitialWorkingDir(
            CompletePath( pcp.GetTargetWorkPath(), targetBase )
        );

        m_loader->LoadBinary(
            m_memorySystem,
            CompletePath( pcp.GetCommand(), targetBase )
        );

        m_codeRange = m_loader->GetCodeRange();

        // mmapに使うヒープを，アドレス空間上で0からバイナリイメージの
        // 直前まで確保する (アドレス0のマップ単位は含まない)
        //u64 heapBase = m_memorySystem->GetPageSize();

        // 鬼斬で予約されていないところから
        u64 heapBase = m_memorySystem->GetReservedAddressRange() + 1;
        m_memorySystem->AddHeapBlock(heapBase, m_loader->GetImageBase()-heapBase);

        InitStack(pcp);

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


void ProcessState::InitStack(const ProcessCreateParam& pcp)
{
    // スタックの確保
    u64 stackMegaBytes = pcp.GetStackMegaBytes();
    if(stackMegaBytes <= 1){
        THROW_RUNTIME_ERROR("Stack size(%d MB) is too small.", stackMegaBytes);
    }

    u64 stackBytes = stackMegaBytes*1024*1024;
    u64 stack = m_memorySystem->MMap(0, stackBytes);

    // 引数の設定
    string targetBase = pcp.GetTargetBasePath();
    m_loader->InitArgs(
        m_memorySystem,
        stack, 
        stackBytes, 
        CompletePath( pcp.GetCommand(), targetBase ),
        pcp.GetCommandArguments() );
}

void ProcessState::InitTargetStdIO(const ProcessCreateParam& pcp)
{
    string targetBase = pcp.GetTargetBasePath();
    string targetWork = 
        CompletePath( pcp.GetTargetWorkPath(), targetBase );


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
            m_virtualSystem->AddFDMap(std_fd[i], std_fd[i], false);
            m_virtualSystem->GetDelayUnlinker()->AddMap(std_fd[i], "HostIO");
        }
        else {
            string filename = CompletePath(std_filename[i], targetWork );
            FILE *fp = fopen( filename.c_str(), omode[i].c_str() );
            if (fp == NULL) {
                THROW_RUNTIME_ERROR("Cannot open file '%s'.", filename.c_str());
            }
            m_autoCloseFile.push_back(fp);
            m_virtualSystem->AddFDMap(std_fd[i], posix_fileno(fp), false);
            m_virtualSystem->GetDelayUnlinker()->AddMap(std_fd[i], filename);
        }
    }
}
