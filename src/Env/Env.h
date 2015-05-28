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


//
// Simulator system environment
// 
//

#ifndef __ONIKIRI_ENV_H
#define __ONIKIRI_ENV_H

#include "Utility/String.h"
#include "Env/Param/ParamExchange.h"

namespace Onikiri
{

    class Environment : public ParamExchange
    {
        std::vector<String> m_cmdLineArgs;
        std::vector<String> m_cfgXmlFiles;
        String m_startupPath;

        u64 m_execBeginTime;
        u64 m_execPeriod;
        String m_versionString;
        String m_sessionName;
        String m_errorMsg;
        bool m_error;
        bool m_dumpSuccess;

        void BeginExec();
        void EndExec();
        void DumpResult();

        class Path : public ParamExchangeChild
        {
            String m_execFilePath;
            String m_startupPath;

            String m_path;
            bool m_useXMLFile;
            bool m_useSimulatorExecFile;
        public:
        
            BEGIN_PARAM_MAP("")
                PARAM_ENTRY( "@Path", m_path )
                PARAM_ENTRY( "@UseXMLFilePath", m_useXMLFile )
                PARAM_ENTRY( "@UseSimulatorExecFilePath", m_useSimulatorExecFile )
            END_PARAM_MAP()

            Path();
            String Get();
            void Initialize( Environment& env );
        };
        Path m_hostWorkPath;
        String m_outputXMLFileName;
        String m_outputXMLLevel;
        String m_outputXMLFilter;

        bool m_outputPrintToSTDOUT;
        String m_outputPrintFileName;
        std::ofstream m_outputPrintStream;

        bool m_suppressInternalMessage;
        bool m_suppressWarning;
        bool m_paramDBInitialized;
    public:
        BEGIN_PARAM_MAP("/Session/")
            PARAM_ENTRY("@Name", m_sessionName)
            PARAM_ENTRY("Result/@ExecutionPeriod", m_execPeriod)
            PARAM_ENTRY("Result/@VersionNumber", m_versionString)
            PARAM_ENTRY("Result/@Error", m_error)
            PARAM_ENTRY("Environment/OutputXML/@FileName", m_outputXMLFileName)
            PARAM_ENTRY("Environment/OutputXML/@Level", m_outputXMLLevel)
            PARAM_ENTRY("Environment/OutputXML/@Filter", m_outputXMLFilter)
            PARAM_ENTRY("Environment/Print/@FileName", m_outputPrintFileName)
            PARAM_ENTRY("Environment/Print/@SuppressInternalMessage", m_suppressInternalMessage)
            PARAM_ENTRY("Environment/Print/@SuppressWarningMessage", m_suppressWarning)
            CHAIN_PARAM_MAP("Environment/HostWorkPath/", m_hostWorkPath)
        END_PARAM_MAP()

        Environment();
        ~Environment();
        void Initialize(int argc, char* argv[], const std::vector<String>& defaultParams = std::vector<String>());
        void Finalize();

        void Print(const std::string& str);
        void Print(const char* str, ...);
        void PrintInternal(const char* str, ...);

        void SetError( bool error, const std::string& msg );
        void SetWarning(bool warning);
        bool IsDumpSuccess();
        void PrintFatalErrorXML();

        const std::vector<String>& GetCmdLineArgs();
        const std::vector<String>& GetCfgXmlFiles();
        String GetStartupPath();
        String GetHostWorkPath();

        bool IsSuppressedInternalMessage();
    };

    extern Environment g_env;

} // namespace Onikiri

#endif

