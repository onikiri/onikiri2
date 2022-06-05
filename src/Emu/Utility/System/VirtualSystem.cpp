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
#include "Emu/Utility/System/VirtualSystem.h"

using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::POSIX;

// const int FDConv::InvalidFD;
// クラスのstatic const 変数のアドレスが取られる場合は，上のような定義が必要だが，
// この定義を置くと，VCのバグで多重定義（リンクエラー）になってしまう
// http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=99610


//
// FDConv
//
FDConv::FDConv()
{
    ExtendFDMap(8);
}

FDConv::~FDConv()
{
}

int FDConv::TargetToHost(int targetFD) const
{
    if (targetFD < 0)
        return InvalidFD;

    if ((size_t)targetFD < m_FDTargetToHostTable.size())
        return m_FDTargetToHostTable[targetFD];
    else
        return InvalidFD;
}

int FDConv::HostToTarget(int hostFD) const
{
    if (hostFD < 0)
        return InvalidFD;

    vector<int>::const_iterator e = find(m_FDTargetToHostTable.begin(), m_FDTargetToHostTable.end(), hostFD);
    if (e != m_FDTargetToHostTable.end())
        return (int)(e - m_FDTargetToHostTable.begin());
    else
        return InvalidFD;
}
bool FDConv::AddMap(int targetFD, int hostFD, const String& hostFileName)
{
    if (targetFD < 0 || hostFD < 0)
        return false;

    if ((size_t)targetFD >= m_FDTargetToHostTable.size())
        ExtendFDMap();

    m_FDTargetToHostTable[targetFD] = hostFD;

    FileContext context;
    context.hostFileName = hostFileName;
    m_targetFD_ContextMap[targetFD] = context;
    return true;
}

bool FDConv::RemoveMap(int targetFD)
{
    if (targetFD < 0)
        return false;

    // targetFDには対応が設定されていない
    if (m_FDTargetToHostTable[targetFD] == InvalidFD)
        return false;

    m_FDTargetToHostTable[targetFD] = InvalidFD;
    m_targetFD_ContextMap[targetFD] = FileContext();
    return true;
}

// targetのfdで空いているものを探す
int FDConv::GetFirstFreeFD()
{
    // 対応表からhostのfdが割り当てられていないものを探す
    vector<int>::iterator e = find(m_FDTargetToHostTable.begin(), m_FDTargetToHostTable.end(), (int)InvalidFD);

    if (e != m_FDTargetToHostTable.end())
        return (int)(e - m_FDTargetToHostTable.begin());
    else {
        // 対応表が埋まっているので拡張する
        int result = (int)m_FDTargetToHostTable.size();
        ExtendFDMap();
        return result;
    }
}

FDConv::FileContext& FDConv::GetContext(int targetFD)
{
    if (m_targetFD_ContextMap[targetFD].hostFileName == "") {
        THROW_RUNTIME_ERROR("Invalid target file descriptor [%d]", targetFD);
    }
    return m_targetFD_ContextMap[targetFD];
}

// m_FDTargetToHostTable のサイズを大きくする
void FDConv::ExtendFDMap()
{
    ExtendFDMap(m_FDTargetToHostTable.size()*2);
}

// m_FDTargetToHostTable のサイズを指定したサイズまで大きくする (小さくすることはできない)
void FDConv::ExtendFDMap(size_t size)
{
    if (size > m_FDTargetToHostTable.size())
        m_FDTargetToHostTable.resize(size, (int)InvalidFD);
}

//
// DelayUnlinker
//
DelayUnlinker::DelayUnlinker()
{
}

DelayUnlinker::~DelayUnlinker()
{
}

bool DelayUnlinker::AddMap(int targetFD, string path)
{
    if(targetFD < 0){
        return false;
    }
    m_targetFDToPathTable[targetFD] = path;

    return true;
}

bool DelayUnlinker::RemoveMap(int targetFD)
{
    if(targetFD < 0){
        return false;
    }
    m_targetFDToPathTable[targetFD] = "";

    return true;
}

bool DelayUnlinker::AddUnlinkPath(string path)
{
    list<string>::iterator i;
    for(i = m_delayUnlinkPathList.begin(); i != m_delayUnlinkPathList.end(); ++i){
        if(*i == path){
            return false;
        }
    }
    m_delayUnlinkPathList.push_back(path);

    return true;
}

bool DelayUnlinker::IfUnlinkable(int targetFD)
{
    map<int, string>::const_iterator i;
    string targetPath = m_targetFDToPathTable[targetFD];
    for(i = m_targetFDToPathTable.begin(); i != m_targetFDToPathTable.end(); ++i){
        if(i->first == targetFD){
            continue;
        }
        if(i->second == targetPath){
            return false;
        }
    }

    bool pathInList = false;
    list<string>::const_iterator j;
    for(j = m_delayUnlinkPathList.begin(); j != m_delayUnlinkPathList.end(); ++j){
        if(*j == targetPath){
            pathInList = true;
        }
    }
    return pathInList;
}

bool DelayUnlinker::RemoveUnlinkPath(string path)
{
    list<string>::iterator i;
    for(i = m_delayUnlinkPathList.begin(); i != m_delayUnlinkPathList.end(); ++i){
        if(*i == path){
            i = m_delayUnlinkPathList.erase(i);
            return true;
        }
    }
    return false;
}


//
// VirtualSystem
//


VirtualSystem::VirtualSystem()
{
    LoadParam();
    m_executedInsnTick = 0;
    m_timeEmulationMode = EmulationModeStrToInt( m_timeEmulationModeStr );
}

VirtualSystem::~VirtualSystem()
{
    ReleaseParam();
    for_each(m_autoCloseFD.begin(), m_autoCloseFD.end(), posix_close);
}

void VirtualSystem::SetInitialWorkingDir(const VirtualPath& dir)
{
    m_cwd = dir;
}

void VirtualSystem::SetCommandFileName(const VirtualPath& absCmdFileName)
{
    m_absCmdFileName = absCmdFileName;
}

bool VirtualSystem::AddFDMap(int targetFD, int hostFD, const String& hostFileName, bool autoclose)
{
    if (!m_fdConv.AddMap(targetFD, hostFD, hostFileName))
        return false;

    if (autoclose)
        AddAutoCloseFD(hostFD);

    return true;
}

int VirtualSystem::GetErrno()
{
    return posix_geterrno();
}

void VirtualSystem::AddAutoCloseFD(int fd)
{
    if (find(m_autoCloseFD.begin(), m_autoCloseFD.end(), fd) == m_autoCloseFD.end())
        m_autoCloseFD.push_back(fd);
}

void VirtualSystem::RemoveAutoCloseFD(int fd)
{
    vector<int>::iterator e = find(m_autoCloseFD.begin(), m_autoCloseFD.end(), fd);
    if (e != m_autoCloseFD.end())
        m_autoCloseFD.erase(e);
}

int VirtualSystem::GetCWD(char* buf, int maxlen)
{
    strncpy(buf, m_cwd.ToGuest().c_str(), maxlen);
    return (int)strlen(buf) + 1; // +1 for NULL?
}

int VirtualSystem::ChDir(const char* path)
{
    if (path[0] == '/') {   // Absolute path
        m_cwd.SetVirtualPath(path);
    }
    else {
        m_cwd.AppendVirtualPath(path);
    }
    return 0;
}

int VirtualSystem::GetPID()
{
    return posix_getpid();
}

int VirtualSystem::GetUID()
{
    return posix_getuid();
}

int VirtualSystem::GetEUID()
{
    return posix_geteuid();
}

int VirtualSystem::GetGID()
{
    return posix_getgid();
}

int VirtualSystem::GetEGID()
{
    return posix_getegid();
}

// Called from Open, and read stream is used in GetDents 
void VirtualSystem::ReadDirectoryEntries(int targetFD, const char* guestFileName)
{
    FDConv::FileContext* fileContext = &m_fdConv.GetContext(targetFD);
    String hostFileName = fileContext->hostFileName;

    // Enumerate files in an opened directory
    namespace fs = boost::filesystem;
    const fs::path path(hostFileName.c_str());
    for (const auto& e : boost::make_iterator_range(fs::directory_iterator(path), {})) {
        HostDirent dirent;
        dirent.isDir = fs::is_directory(e);
        dirent.name = e.path().filename().string();

        // Windows stat always returns 0 for "ino", but 
        // some guest programs recognize "ino 0" as a invalid file,
        // so a dummy number is used
        dirent.ino = 1;

        //HostStat stat;
        //Stat(String(guestFileName) + "/" + dirent.name, &stat);
        //dirent.ino = stat.st_ino;

        fileContext->dirStream.push_back(dirent);
    }
}

int VirtualSystem::Open(const char* filename, int oflag)
{
    String hostFileName = GetHostPath(filename);
    int hostFD = posix_open(hostFileName.c_str(), oflag, POSIX_S_IWRITE | POSIX_S_IREAD);

    // FDの対応表に追加
    if (hostFD == -1) {
        return -1;
    }
    int targetFD = m_fdConv.GetFirstFreeFD();
    AddFDMap(targetFD, hostFD, hostFileName, true);
    m_delayUnlinker.AddMap(targetFD, hostFileName);


    // Open a directory stream.
    // Read directory entries 
    if (oflag & POSIX_O_DIRECTORY) {
        ReadDirectoryEntries(targetFD, filename);
    }
    return targetFD;
}

int VirtualSystem::Dup(int fd)
{
    int hostFD = FDTargetToHost(fd);
    int dupHostFD = posix_dup(hostFD);
    // FDの対応表に追加
    if (dupHostFD != -1) {
        int targetFD = m_fdConv.GetFirstFreeFD();
        String hostFileName = m_fdConv.GetContext(fd).hostFileName;
        AddFDMap(targetFD, dupHostFD, hostFileName, true);
        m_delayUnlinker.AddMap(targetFD, hostFileName);
        return targetFD;
    }
    else {
        return -1;
    }
}


int VirtualSystem::Read(int fd, void* buffer, unsigned int count)
{
    int hostFD = m_fdConv.TargetToHost(fd);
    return posix_read(hostFD, buffer, count);
}

int VirtualSystem::Write(int fd, void* buffer, unsigned int count)
{
    int hostFD = m_fdConv.TargetToHost(fd);
    return posix_write(hostFD, buffer, count);
}

int VirtualSystem::Close(int fd)
{
    if (fd < 3){
        // Block closing stdin/stdout/stderr
        // When files are connected to stdin/stdout/stderr, they will be closed 
        // in ProcessState, and thus, they must not be closed here
        return 0;
    }

    int hostFD = m_fdConv.TargetToHost(fd);
    int result = posix_close(hostFD);
    if (result != -1) {
        // closeに成功したら自動クローズリストから除外
        RemoveAutoCloseFD(hostFD);

        String hostFileName = m_fdConv.GetContext(fd).hostFileName;
        m_fdConv.RemoveMap(fd);
#ifdef HOST_IS_WINDOWS
        if( m_delayUnlinker.IfUnlinkable(fd) ){
            m_delayUnlinker.RemoveUnlinkPath(hostFileName);
            posix_unlink(hostFileName.c_str());
        }
#endif
        m_delayUnlinker.RemoveMap(fd);
    }

    return result;
}

// readlink at
int VirtualSystem::ReadLinkAt(int targetFD, const char* pathname, void *buffer, unsigned int count) {
    if (string("/proc/self/exe") == pathname) {
        if (m_absCmdFileName.ToGuest().size() >= count) {
            return -1;
        }
        ::strncpy(static_cast<char*>(buffer), m_absCmdFileName.ToGuest().c_str(), sizeof(char)*count-1);
        return 0;
    }
    else {
        THROW_RUNTIME_ERROR(
            "'readlinkat' does not support reading other than '/proc/self/exe', "
            "but '%s' is specified.", pathname
        );
        return -1;
    }
}

int VirtualSystem::FStat(int fd, HostStat* buffer)
{
    int hostFD = FDTargetToHost(fd);
    return posix_fstat(hostFD, buffer);
}
int VirtualSystem::FStatat(int fd, const char* path, HostStat* s) 
{
    String workingPath = FDTargetToFileName(fd);
    String targetPath = workingPath + "/" + String(path);
    return posix_stat(targetPath.c_str(), s);
}

int VirtualSystem::Stat(const char* path, posix_struct_stat* s)
{
    return posix_stat(GetHostPath(path).c_str(), s);
}
int VirtualSystem::LStat(const char* path, posix_struct_stat* s)
{
    return posix_lstat(GetHostPath(path).c_str(), s);
}

s64 VirtualSystem::LSeek(int fd, s64 offset, int whence)
{
    int hostFD = FDTargetToHost(fd);
    return posix_lseek(hostFD, offset, whence);
}

int VirtualSystem::Access(const char* path, int mode)
{
    return posix_access(GetHostPath(path).c_str(), mode);
}

int VirtualSystem::Unlink(const char* path)
{
    /* unlink について
    Unix 上において、いずれかのプロセスが open している
    ファイルに対して unlink すると、すべてのプロセスが
    close した時点でファイルが削除される。
    一方、Windows では open しているファイルに対する unlink は
    エラーを返すので挙動が変わってしまう。
    ここでは Windows の挙動を Unix に合わせるため DelayUnlinker クラスを用いる
    */
    int unlinkerr = posix_unlink(GetHostPath(path).c_str());
#ifdef HOST_IS_WINDOWS
    if( posix_geterrno() == 0xd ){
        m_delayUnlinker.AddUnlinkPath(GetHostPath(path));
        return 0;
    }
#endif
    return unlinkerr;
}

int VirtualSystem::Rename(const char* oldpath, const char* newpath)
{
    return posix_rename(
        m_cwd.CompleteInHost(oldpath).c_str(),
        m_cwd.CompleteInHost(newpath).c_str()
    );
}

int VirtualSystem::Truncate(const char* path, s64 length)
{
    return posix_truncate(GetHostPath(path).c_str(), length);
}

int VirtualSystem::FTruncate(int fd, s64 length)
{
    int hostFD = FDTargetToHost(fd);
    return posix_ftruncate(hostFD, length);
}

int VirtualSystem::MkDir(const char* path, int mode)
{
    namespace fs = filesystem;
    if (fs::create_directory( GetHostPath(path).c_str() ))
        return 0;
    else 
        return -1;
}

// Returns a single HostDirent entry from targetFD
// getdents64 cannot be available in Windows/Cygwin directly,
// so this system call is emulated in software.
// Return:
//   -1: error
//    1: success 
//    0: reach to the end of directory stream
// A stream is loaded in ReadDirectoryEntries called from Open
int VirtualSystem::GetDents(int targetFD, HostDirent* dirent)
{
    FDConv::FileContext* context = &m_fdConv.GetContext(targetFD);
    if (context->hostFileName == "") {
        return -1;
    }

    if (context->dirStream.size() == 0) {
        return 0;
    }

    *dirent = context->dirStream.front();
    context->dirStream.pop_front();
    return 1;
}

String VirtualSystem::GetHostPath(const char* targetPath)
{
    return m_cwd.CompleteInHost(targetPath);
}

//
// 時刻の取得
//

int VirtualSystem::EmulationModeStrToInt( const std::string& str )
{
    if(str == "Host"){
        return TIME_HOST;
    }
    else if(str == "Fixed"){
        return TIME_FIXED;
    }
    else if(str == "InstructionBased"){
        return TIME_INSTRUCTION;
    }
    else{
        THROW_RUNTIME_ERROR( 
            "'Emulator/System/Time/@EmulationMode' must be one of the following strings: "
            "'Host', 'Fixed', 'InstructionBased'" 
        );
        return -1;
    }
}

static const int INSN_PER_SEC = 2000000000;

s64 VirtualSystem::GetTime()
{
    switch(m_timeEmulationMode){
    default:
    case TIME_HOST:
        return time(NULL);
    case TIME_FIXED:
        return m_unixTime;
    case TIME_INSTRUCTION:
        return m_unixTime + m_executedInsnTick/INSN_PER_SEC;
    }
}

s64 VirtualSystem::GetClock()
{
    switch(m_timeEmulationMode){
    default:
    case TIME_HOST:
        return clock();
    case TIME_FIXED:
        return 0;
    case TIME_INSTRUCTION:
        return m_executedInsnTick/(INSN_PER_SEC/1000);
    }
}

