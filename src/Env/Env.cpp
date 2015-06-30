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
#include "Env/Env.h"
#include <boost/filesystem.hpp>

namespace Onikiri
{
    Environment g_env;
}

using namespace Onikiri;
using namespace std;

Environment::Path::Path()
{
    m_useXMLFile = false;
    m_useSimulatorExecFile = false;
}

static String RemoveFileName(const String& path)
{
    String ret;
    int indexBs = (int)path.rfind( "\\" );
    int indexSl = (int)path.rfind( "/" );
    int indexPath = indexSl > indexBs ? indexSl : indexBs;
    if(indexPath == -1)
        ret = "./";
    else
        ret.assign(path.begin(), path.begin() + indexPath + 1);
    return ret;
}

String Environment::Path::Get()
{
    if(m_useSimulatorExecFile){
        return m_execFilePath + m_path;
    }
    else if(m_useXMLFile){
        const ParamXMLPath path = GetChildRootPath() + "/@UseXMLFilePath";
        String xmlFile;
        if(!g_paramDB.GetSourceXMLFile(path, xmlFile)){
            THROW_RUNTIME_ERROR("UseXMLFilePath is enabled and parameter is not found in XML file.");
        }
        return RemoveFileName( xmlFile ) + m_path;
    }

    return m_path;
}

void Environment::Path::Initialize( Environment& env )
{
    m_startupPath  = env.GetStartupPath();
    m_execFilePath = RemoveFileName( env.GetCmdLineArgs()[0] );
}

// ----
Environment::Environment()
{
    m_cmdLineArgs.clear();
    m_execBeginTime = 0;
    m_execPeriod = 0;
    m_error = false;
    m_dumpSuccess = false;
    m_outputPrintToSTDOUT = true;
    m_suppressInternalMessage = false;
    m_suppressWarning = false;
    m_paramDBInitialized = false;

    m_versionString.format(
        "%x.%02x", 
        ONIKIRI_VERSION >> ONIKIRI_VERSION_DECIMAL_POINT,
        ONIKIRI_VERSION & ((1<<ONIKIRI_VERSION_DECIMAL_POINT)-1) );
}

Environment::~Environment()
{
}


void Environment::Initialize(
    int argc, 
    char* argv[],
    const std::vector<String>& defaultParams )
{
    if( argc <= 1 ){
        THROW_RUNTIME_ERROR( "No input parameter is passed." );
    }

    m_startupPath = 
        boost::filesystem::initial_path().string() + "/";

    if(!g_paramDB.Initialize( m_startupPath )){
        THROW_RUNTIME_ERROR("Initializing ParamDB failed.");
    }
    m_paramDBInitialized = true;

    for(size_t i = 0; i < defaultParams.size(); i++){
        g_paramDB.AddUserDefaultParam( defaultParams[i] );
    }

    for( int i = 0; i < argc; i++){
        m_cmdLineArgs.push_back(argv[i]);
    }
    g_paramDB.LoadParameters( m_cmdLineArgs );

    LoadParam();
    m_hostWorkPath.Initialize(*this);

    m_outputPrintToSTDOUT = m_outputPrintFileName == "";
    if(!m_outputPrintToSTDOUT){
        String printFileName = GetHostWorkPath() + m_outputPrintFileName;
        m_outputPrintStream.open( printFileName );
        if(!m_outputPrintStream){
            m_outputPrintToSTDOUT = true;   // It is required to output below message.
            THROW_RUNTIME_ERROR( "Could not open '%s'.", m_outputPrintFileName.c_str() );
        }
    }

    SuppressWaning( m_suppressWarning );
    PrintInternal( "Onikiri Version %s\n", m_versionString.c_str() );
    BeginExec();
}

void Environment::Finalize()
{
    EndExec();
    ReleaseParam();

    if(m_paramDBInitialized){
        DumpResult();
        m_dumpSuccess = true;
    }

    g_paramDB.Finalize();
}

void Environment::DumpResult()
{
    if( m_error ){
        // Set an error message to the following path.
        g_paramDB.Set( "/Session/Result/Error/Message/text()", m_errorMsg );
    }

    // Dump XML data to a string.
    String resultStr = 
        g_paramDB.DumpResultXML( m_outputXMLLevel, m_outputXMLFilter );

    bool outputToFile = false;
    if(m_outputXMLFileName != ""){
        std::ofstream ofs;
        String fileName = GetHostWorkPath() + m_outputXMLFileName;
        ofs.open( fileName );
        if(ofs){
            ofs << resultStr.c_str();
            ofs.close();
            outputToFile = true;
        }
        else{
            Print("Could not open output xml file '%s'.\n", fileName.c_str());
        }
    }

    if(!outputToFile){
        // Not used "Print"
        printf( "\n\n%s", resultStr.c_str() );
    }
}

void Environment::Print(const string& str)
{
    return Print( str.c_str() );
}

void Environment::Print(const char* fmt, ...)
{
    String str;
    va_list arg;
    va_start(arg, fmt);
    str.format_arg(fmt, arg);
    va_end(arg);

    // 
    if( m_outputPrintToSTDOUT )
        printf( "%s", str.c_str() );
    else
        m_outputPrintStream << str;
}

void Environment::PrintInternal(const char* fmt, ...)
{
    String str;
    va_list arg;
    va_start(arg, fmt);
    str.format_arg(fmt, arg);
    va_end(arg);

    if(!m_suppressInternalMessage)
        Print(str.c_str());
}


const vector<String>& Environment::GetCfgXmlFiles()
{
    return m_cfgXmlFiles;
}

const vector<String>& Environment::GetCmdLineArgs()
{
    return m_cmdLineArgs;
}

String Environment::GetHostWorkPath()
{
    return m_hostWorkPath.Get();
}

String Environment::GetStartupPath()
{
    return m_startupPath;
}

void Environment::BeginExec()
{
    m_execBeginTime = clock();
}

void Environment::EndExec()
{
    m_execPeriod =
        ((u64)clock() - m_execBeginTime) * 1000 / CLOCKS_PER_SEC;
}

void Environment::SetError( bool error, const std::string& msg )
{
    m_error = error;
    m_errorMsg = msg;
}

bool Environment::IsDumpSuccess()
{
    return m_dumpSuccess;
}

void Environment::PrintFatalErrorXML()
{
    Print(
        "<?xml version='1.0' encoding='UTF-8'?>\n"
        "<Session>\n"
        "  <Result Error='1'>\n"
        "</Session>\n"
    );
}

bool Environment::IsSuppressedInternalMessage()
{
    return m_suppressInternalMessage;
};
