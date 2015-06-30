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
#include "DebugStub.h"

using namespace Onikiri;
using namespace std;
using namespace boost::asio;
using ip::tcp;

// #define GDB_DEBUG

DebugStub::DebugStub(SystemBase::SystemContext* context, int pid) :
    m_acc(m_ioService, tcp::endpoint(tcp::v4(), (unsigned short)context->debugParam.debugPort))
{
    ASSERT(context->targetArchitecture == "AlphaLinux", "GDB Debug mode is currently available on AlphaLinux only.");
    ASSERT(context->threads.GetSize() == 1, "Multithread GDB Debugging is not supported.");

    m_context = context;
    m_pid = pid;
    m_stopExec = true;
    m_stopCount = 0;
    g_env.PrintInternal("Waiting for host GDB access...");
    m_acc.accept(*m_iostream.rdbuf());
    g_env.PrintInternal(" connected.\n");
    while(m_stopExec) {
        if( GetStartChar() ){
            if( !GetStream() ) return;
            ParsePacket();
            ExecDebug();
        }
    }
}

DebugStub::~DebugStub()
{
    SendPacket("W00");
}

//
// Communicating functions
//

bool DebugStub::GetStartChar()
{
    char ch;
    while(m_iostream.rdbuf()->available()){
        m_iostream.get(ch);
        if(ch == '$') return true;
    }
    return false;
}

bool DebugStub::GetStream()
{
    char ch;
    int checksum = 0;
    int sum = 0;
    stringstream ss;
    m_streamBuffer.clear();
    while (m_iostream.get(ch)) {
        if(ch == '#') break;
        sum += ch;
        m_streamBuffer += ch;
    }
    m_iostream.get(ch);
    ss << ch;
    m_iostream.get(ch);
    ss << ch;
    ss >> hex >> checksum;
#ifdef GDB_DEBUG
    cout << "received:\"" << m_streamBuffer << "\"" << endl;
#endif
    if( (sum&0xff) == checksum ) {
        m_iostream << '+';
        return true;
    }
    else {
        m_iostream << '-';
        return false;
    }
}

void DebugStub::ParsePacket()
{
    size_t pos;
    m_packet.clear();
    switch(m_packet.letter = m_streamBuffer.at(0)){
    case ('?'): // Indicate the reason the target halted
        break;
    case ('q'): // General query
        pos = m_streamBuffer.find(':');
        m_packet.command = m_streamBuffer.substr(1,pos-1);
        while(pos != m_streamBuffer.npos){
            size_t nextpos;
            nextpos = m_streamBuffer.find(';',pos+1);
            m_packet.params.push_back(m_streamBuffer.substr(pos+1,nextpos-pos-1));
            pos = nextpos;
        }
        break;
    case ('H'): // Set thread
        m_packet.command = m_streamBuffer.at(1);
        m_packet.params.push_back(m_streamBuffer.substr(2));
        break;
    case ('g'): // Read general registers
        break;
    case ('G'): // Write general registers
        m_packet.command = m_streamBuffer.substr(1);
        break;
    case ('p'): // Read the value of specified register
        m_packet.command = m_streamBuffer.substr(1);
        break;
    case ('P'): // Write specified register
        pos = m_streamBuffer.find('=');
        m_packet.command = m_streamBuffer.substr(1,pos-1);
        m_packet.params.push_back(m_streamBuffer.substr(pos+1));
        break;
    case ('m'): // Read memory
        pos = m_streamBuffer.find(',');
        m_packet.command = m_streamBuffer.substr(1,pos-1);
        m_packet.params.push_back(m_streamBuffer.substr(pos+1));
        break;
    case ('X'): // Write memory
    case ('M'): // Write memory
        pos = m_streamBuffer.find(',');
        m_packet.command = m_streamBuffer.substr(1,pos-1);
        while(pos != m_streamBuffer.npos){
            size_t nextpos;
            nextpos = m_streamBuffer.find(':',pos+1);
            m_packet.params.push_back(m_streamBuffer.substr(pos+1,nextpos-pos-1));
            pos = nextpos;
        }
        break;
    case ('v'): // Resume
        pos = m_streamBuffer.find(';');
        m_packet.command = m_streamBuffer.substr(1,pos-1);
        while(pos != m_streamBuffer.npos){
            size_t nextpos;
            nextpos = m_streamBuffer.find(';',pos+1);
            m_packet.params.push_back(m_streamBuffer.substr(pos+1,nextpos-pos-1));
            pos = nextpos;
        }
        break;
    case ('Z'): // Insert breakpoint
    case ('z'): // Remove breakpoint
        pos = m_streamBuffer.find(',');
        m_packet.command = m_streamBuffer.substr(1,pos-1);
        while(pos != m_streamBuffer.npos){
            size_t nextpos;
            nextpos = m_streamBuffer.find(',',pos+1);
            m_packet.params.push_back(m_streamBuffer.substr(pos+1,nextpos-pos-1));
            pos = nextpos;
        }
        break;
    default:
        break;
    }

}

void DebugStub::ExecDebug()
{
    size_t readnum = 0;
    string readstr = "";
    u64 addr;
    u64 value = 0;
    MemAccess access;
    bool validAccess = true;
    switch(m_packet.letter){
    case ('?'): // Indicate the reason the target halted
        SendPacket("T05thread:00;");
        break;
    case ('q'): // General query
        if( m_packet.command == "Supported" ){
            if( m_packet.params.at(0) == "qRelocInsn+" ){
                SendPacket("PacketSize=1000");
            }
            else{
                SendPacket("");
            }
        }
        else if ( m_packet.command == "fThreadInfo" ){
            SendPacket("m0");
        }
        else if ( m_packet.command == "sThreadInfo" ){
            SendPacket("l");
        }
        else if ( m_packet.command == "C")
        {
            SendPacket("QC1");
        }
        else if ( m_packet.command == "Attached")
        {
            SendPacket("");
        }
        else if ( m_packet.command == "TStatus")
        {
            SendPacket("");
        }
        else if ( m_packet.command == "TfV")
        {
            SendPacket( U64ToHexStr(0, 8) + ":" + U64ToHexStr(0, 8));
        }
        else if ( m_packet.command == "TsV")
        {
            SendPacket("l");
        }
        else if ( m_packet.command == "TfP")
        {
            SendPacket("l");
        }
        else if ( m_packet.command == "Offsets")
        {
            SendPacket("Text=0000000000000000;Data=0000000000000000;Bss=0000000000000000");
        }
        else if ( m_packet.command == "Symbol")
        {
            SendPacket("");
        }
        else
        {
            SendPacket("");
        }
        break;
    case ('H'): // Set thread
        if ( m_packet.command == "g" && m_packet.params.at(0) == "0" ){
            SendPacket("OK");
        }
        else if ( m_packet.command == "g" && m_packet.params.at(0) == "1" ){
            SendPacket("E22");
        }
        else if ( m_packet.command == "c" && m_packet.params.at(0) == "-1" ){
            SendPacket("OK");
        }
        else{
            SendPacket("");
        }
        break;
    case ('g'): // Read general registers
        readnum = m_context->architectureStateList.at(m_pid).registerValue.capacity();
        for(size_t i = 0; i < readnum; i++){
            readstr += U64ToHexStr(GetRegister((int)i), 8); // TODO: 32bit register
        }
        SendPacket(readstr);
        break;
    case ('G'): // Write general registers
        readnum = m_context->architectureStateList.at(m_pid).registerValue.capacity();
        for(size_t i = 0; i < readnum; i++){
            u64 value = 0;
            for (int j=0; j < 8; j++)
            {
                value += HexStrToU64(m_packet.command.substr(16*i+(j<<1),2)) << (j*8);
            }
            SetRegister((int)i,value);
        }
        SendPacket("OK");
        break;
    case ('p'): // Read the value of specified register
        SendPacket(U64ToHexStr(GetRegister((int)HexStrToU64(m_packet.command)), 8)); // TODO: 32bit register
        break;
    case ('P'): // Write specified register
        for (int i=0; i < 8; i++)
        {
            value += HexStrToU64(m_packet.params.at(0).substr(i<<1,2)) << (i*8);
        }
        SetRegister((int)HexStrToU64(m_packet.command), value);
        SendPacket("OK");
        break;
    case ('m'): // Read memory
        readnum = (int)HexStrToU64(m_packet.params.at(0));
        addr = HexStrToU64(m_packet.command);
        while(readnum){
            size_t readSize = (readnum > 8) ? 8 : readnum;
            access.address = Addr(m_pid, 0, addr);
            access.size = (int)readSize;
            readstr += U64ToHexStr(GetMemory(&access), (int)readSize);
            readnum -= readSize;
            addr += readSize;
            if (access.result != MemAccess::MAR_SUCCESS)
            {
                validAccess = false;
                break;
            }
        }
        if (validAccess){
            SendPacket(readstr);
        } else {
            SendPacket("E14");
        }
        break;
    case ('X'): // Write memory
        readnum = (int)HexStrToU64(m_packet.params.at(0));
        addr = HexStrToU64(m_packet.command);
        for(size_t i = 0; readnum != 0;){
            size_t readSize = (readnum > 8) ? 8 : readnum;
            access.address = Addr(m_pid, 0, addr);
            access.size = (int)readSize;
            access.value = ParseBinary(m_packet.params.at(1).substr(i,readSize));
            SetMemory(&access);
            readnum -= readSize;
            addr += readSize;
            i += readSize;
            if (access.result != MemAccess::MAR_SUCCESS)
            {
                validAccess = false;
                break;
            }
        }
        if (validAccess){
            SendPacket("OK");
        } else {
            SendPacket("E14");
        }
        break;
    case ('M'): // Write memory
        readnum = (int)HexStrToU64(m_packet.params.at(0));
        addr = HexStrToU64(m_packet.command);
        while(readnum){
            size_t readSize = (readnum > 8) ? 8 : readnum;
            u64 value = 0;
            for (size_t i=0; i < readSize; i++)
            {
                value += HexStrToU64(m_packet.params.at(1).substr(i<<1,2)) << (i*8);
            }
            access.address = Addr(m_pid, 0, addr);
            access.size = (int)readSize;
            access.value = value;
            SetMemory(&access);
            readnum -= readSize;
            addr += readSize;
            if (access.result != MemAccess::MAR_SUCCESS)
            {
                validAccess = false;
                break;
            }
        }
        if (validAccess){
            SendPacket("OK");
        } else {
            SendPacket("E14");
        }
        break;
    case ('v'): // Resume
        if(m_packet.command == "Cont?"){
            SendPacket("vCont;c;C;s;S");
        }
        else if (m_packet.command == "Cont"){
            if (m_packet.params.at(0) == "c"){
                m_stopExec = false;
            }
            else if (m_packet.params.at(0) == "s:0"){
                m_stopExec = false;
                m_stopCount = 1;
            }
        }
        else if (m_packet.command == "Kill"){
            THROW_RUNTIME_ERROR("Terminated via gdb.");
        }
        break;
    case ('c'): // Continue
        m_stopExec = false;
        break;
    case ('s'): // Step
        m_stopExec = false;
        m_stopCount = 1;
        break;
    case ('t'): // Terminate?
        m_stopExec = true;
        SendPacket("");
        break;
    case ('k'): // Kill
        THROW_RUNTIME_ERROR("Terminated via gdb.");
        break;
    case ('Z'): // Insert breakpoint
        if(m_packet.command == "0"){
            m_breakpoint.insert(pointpair(HexStrToU64(m_packet.params.at(0)),(int)HexStrToU64(m_packet.params.at(1))));
            SendPacket("OK");
        }
        else {
            WatchList* watchList;
            if (m_packet.command == "2") {
                watchList = &m_watchWrite;
            } 
            else if (m_packet.command == "3") {
                watchList = &m_watchRead;
            }
            else if (m_packet.command == "4") {
                watchList = &m_watchAccess;
            }
            else{
                SendPacket("");
                break;
            }
            watchList->push_back(pointpair(HexStrToU64(m_packet.params.at(0)),(int)HexStrToU64(m_packet.params.at(1))));
            SendPacket("OK");
        }
        break;
    case ('z'): // Remove breakpoint
        if(m_packet.command == "0"){
            Pointmap::iterator i = m_breakpoint.find(HexStrToU64(m_packet.params.at(0)));
            if (i != m_breakpoint.end() && i->second == (int)HexStrToU64(m_packet.params.at(1)))
            {
                m_breakpoint.erase(i);
            }
            SendPacket("OK");
        }
        else {
            WatchList* watchList;
            if (m_packet.command == "2")
            {
                watchList = &m_watchWrite;
            } 
            else if (m_packet.command == "3")
            {
                watchList = &m_watchRead;
            }
            else if (m_packet.command == "4"){
                watchList = &m_watchAccess;
            }
            else{
                SendPacket("");
                break;
            }

            for (WatchList::iterator i = watchList->begin(); i != watchList->end();)
            {
                if (i->first == HexStrToU64(m_packet.params.at(0)) &&
                    i->second == (int)HexStrToU64(m_packet.params.at(1)))
                {
                    i =watchList->erase(i);
                }
                else{
                    i++;
                }
            }
            SendPacket("OK");
        }
        break;
    default:
        SendPacket("");
        break;
    }

}

void DebugStub::SendPacket(string packet)
{
    for(int i=0; i < 3; i++){ // try 3 times and then give up
        int sum = 0;
        stringstream ss;
        for(unsigned int i = 0; i < packet.size(); i++){
            sum += packet.at(i);
        }
        ss << setw(2) << setfill('0') << hex << (sum & 0xff);
#ifdef GDB_DEBUG
        cout << "sent:\"" << packet << "\"" << endl;
#endif
        m_iostream << "$" << packet << "#" << ss.str();

        char ch;
        bool thru = false;
        do{
            m_iostream.get(ch);
            if(ch == '$') thru = true;
            if(ch == '#' && thru) thru = false;
        } while ((ch != '+' && ch != '-') || thru);
        if(ch == '+') break;
    }

}

void DebugStub::OnExec(EmulationDebugOp* op)
{
    do { // Wait for commands from gdb in this loop
        if(m_stopCount > 0){ // step exec
            if(m_stopCount == 1){
                m_stopExec = true;
                SendPacket("S05");
            }
            m_stopCount--;
        }
        if(!m_stopExec){ // Software Breakpoint
            if (m_breakpoint.find(m_context->architectureStateList[m_pid].pc.address) != m_breakpoint.end()) // Z0
            {
                m_stopExec = true;
                SendPacket("S05");
            }
            else if (op->GetOpInfo()->GetOpClass().IsLoad()) // Z3, Z4
            {
                MemAccess access = op->GetMemAccess();
                for (WatchList::iterator i = m_watchRead.begin(); i != m_watchRead.end(); i++)
                {
                    if (IsAddressOverlap(access,*i))
                    {
                        m_stopExec = true;
                        SendPacket("S05");
                    }
                }
                for (WatchList::iterator i = m_watchAccess.begin(); i != m_watchAccess.end(); i++)
                {
                    if (IsAddressOverlap(access,*i))
                    {
                        m_stopExec = true;
                        SendPacket("S05");
                    }
                }
            }
            else if (op->GetOpInfo()->GetOpClass().IsStore()) // Z2, Z4
            {
                MemAccess access = op->GetMemAccess();
                for (WatchList::iterator i = m_watchWrite.begin(); i != m_watchWrite.end(); i++)
                {
                    if (IsAddressOverlap(access,*i))
                    {
                        m_stopExec = true;
                        SendPacket("S05");
                    }
                }
                for (WatchList::iterator i = m_watchAccess.begin(); i != m_watchAccess.end(); i++)
                {
                    if (IsAddressOverlap(access,*i))
                    {
                        m_stopExec = true;
                        SendPacket("S05");
                    }
                }
            }
        }
        if( GetStartChar() ){
            if( !GetStream() ) return;
            ParsePacket();
            ExecDebug();
        }
    }while(m_stopExec);
}

//
// Utility functions
//

u64 DebugStub::GetRegister( int regNum )
{
    // TODO: set correct PC on every ISA
    if (regNum == 0x42)
    {
        return 0;
    }
    return (regNum != 0x40) ? m_context->architectureStateList[m_pid].registerValue[regNum] : m_context->architectureStateList[m_pid].pc.address;
}

void DebugStub::SetRegister( int regNum, u64 value )
{
    // TODO: set correct PC on every ISA
    if(regNum != 0x40){
        m_context->architectureStateList[m_pid].registerValue[regNum] = value;
    }
    else {
        m_context->architectureStateList[m_pid].pc.address = value;
    }
}

u64 DebugStub::GetMemory( MemAccess* access )
{
    m_context->emulator->GetMemImage()->Read(access);
    return access->value;
}

void DebugStub::SetMemory( MemAccess* access )
{
    m_context->emulator->GetMemImage()->Write(access);
    return;
}

u64 DebugStub::HexStrToU64(string str)
{
    stringstream ss;
    u64 retVal;

    ss << str;
    ss >> hex >> retVal;
    return retVal;
}

string DebugStub::U64ToHexStr( u64 val, int num )
{
    stringstream ss;
    for(int i = 0; i < num; i++){
        ss << setw(2) << setfill('0') << hex << (val & 0xff);
        val >>= 8;
    }
    return ss.str();
}

u64 DebugStub::ParseBinary(string binStr)
{
    u64 value = 0;
    int numByte = 0;
    for (unsigned int i = 0; i < binStr.length(); i++)
    {
        // '#', '$' and '}' are escaped by '}'.
        // The original character is XORed with 0x20.
        if (binStr[i] == '}')
        {
            i++;
            value += ((unsigned char)binStr[i]^0x20) << (numByte*8);
        }
        else
        {
            value += (unsigned char)binStr[i] << (numByte*8);
        }
        numByte++;
    }
    return value;
}

bool DebugStub::IsAddressOverlap(MemAccess access, pointpair watchpoint)
{
    return ((access.address.address <= watchpoint.first) && 
        (watchpoint.first < access.address.address+access.size)) ||
           ((watchpoint.first <= access.address.address) && 
        (access.address.address < watchpoint.first+watchpoint.second));
}
