// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Masahiro Goshima.
// Copyright (c) 2005-2017 Ryota Shioya.
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


#ifndef SIM_SYSTEN_EMULATION_DEBUG_SYSTEM_DEBUG_STUB_DEBUG_STUB_H
#define SIM_SYSTEN_EMULATION_DEBUG_SYSTEM_DEBUG_STUB_DEBUG_STUB_H

#include "Sim/System/SystemBase.h"
#include "Sim/System/EmulationDebugSystem/EmulationDebugOp.h"

#include "SysDeps/Boost/asio.h"

namespace Onikiri{
    class DebugStub
    {
        boost::asio::io_service m_ioService;
        boost::asio::ip::tcp::acceptor m_acc;
        boost::asio::ip::tcp::iostream m_iostream;
        std::string m_streamBuffer;
        SystemBase::SystemContext* m_context;
        int m_pid;
        bool m_stopExec;
        int m_stopCount;
        bool m_reg64;   // This flag is set if register width is 64 bits

        typedef std::pair<u64,int> pointpair;
        typedef std::map<u64,int> Pointmap;
        typedef std::list<pointpair> WatchList;

        Pointmap m_breakpoint;
        WatchList m_watchWrite;
        WatchList m_watchRead;
        WatchList m_watchAccess;

        struct DebugPacket
        {
            char letter;
            std::string command;
            std::vector<std::string> params;
            void clear(){
                letter = 0;
                command = "";
                params.clear();
            }
            std::string GetParams() {
                std::string result = "";
                for (auto s : params) result += s;
                return result;
            }
        };
        DebugPacket m_packet;

        // Communicating functions
        bool GetStartChar();
        bool GetStream();
        void ParsePacket();
        void ExecDebug();
        void SendPacket(std::string packet);

        // Utility functions
        u64 GetRegister(int i);
        void SetRegister( int i, u64 value );
        size_t GetTotalGeneralRegisterNum();
        u64 GetMemory(MemAccess* access);
        void SetMemory(MemAccess* access);
        u64 HexStrToU64(std::string str);
        u32 HexStrToU32(std::string str);
        std::string U64ToHexStr(u64 val, int num);
        std::string U32ToHexStr(u32 val, int num);
        u64 ParseBinary(std::string binStr);
        bool IsAddressOverlap(MemAccess access, pointpair watchpoint);
    public:
        DebugStub(SystemBase::SystemContext* context, int pid);
        ~DebugStub();

        void OnExec(EmulationDebugOp* op);
    };

}

#endif
