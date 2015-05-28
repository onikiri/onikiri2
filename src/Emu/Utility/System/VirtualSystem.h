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


#ifndef __EMULATORUTILITY_VIRTUALSYSTEM_H__
#define __EMULATORUTILITY_VIRTUALSYSTEM_H__

#include "SysDeps/posix.h"

namespace Onikiri {
    namespace EmulatorUtility {
        typedef POSIX::posix_struct_stat HostStat;

        // targetとhost間のFD変換を行うクラス
        class FDConv
        {
        public:
            static const int InvalidFD = -1;

            FDConv();
            ~FDConv();

            // targetのFDとhostのFDの変換を行う
            int TargetToHost(int targetFD) const;
            int HostToTarget(int hostFD) const;

            // FDの対応を追加
            bool AddMap(int targetFD, int hostFD);

            // FDの対応を削除
            bool RemoveMap(int targetFD);

            // 空いている target のFDを返す
            int GetFirstFreeFD();
        private:
            void ExtendFDMap();
            void ExtendFDMap(size_t size);

            // targetのfdをhostのfdに変換する表．hostのfdが割り当てられていなければInvalidFDが入っている
            std::vector<int> m_FDTargetToHostTable;
        };
        
        // プロセスが open 中のファイルを unlink したときの挙動を
        // Unix に合わせるためのクラス
        // 詳細は VirtualSystem::Unlink に記述
        class DelayUnlinker
        {
        public:
            DelayUnlinker();
            ~DelayUnlinker();

            // TargetFD<>path の対応を追加
            bool AddMap(int targetFD, std::string path);
            // TargetFD<>path の対応を削除
            bool RemoveMap(int targetFD);
            // TargetFD<>path の対応するパスを取得
            std::string GetMapPath(int targetFD);
            // 削除候補の path を追加
            bool AddUnlinkPath(std::string path);
            // 削除候補の path を Unlink するかどうか
            bool IfUnlinkable(int targetFD);
            // 削除候補の path を削除
            bool RemoveUnlinkPath(std::string path);
        private:
            std::map<int, std::string> m_targetFDToPathTable;
            std::list<std::string> m_delayUnlinkPathList;
        };

        // targetのための仮想システム（ファイル等）を提供
        //  - targetのプロセス内のfdとホストのfdの変換を行う
        //  - target毎にworking directoryを持つ
        //  - 時刻の取得
        class VirtualSystem : public ParamExchange
        {
        public:

            VirtualSystem();
            ~VirtualSystem();

            // VirtualSystemでOpenしていないファイルを明示的にFDの変換表に追加する(stdin, stdout, stderr等)
            bool AddFDMap(int targetFD, int hostFD, bool autoclose = false);
            // ターゲットの作業ディレクトリを設定する
            void SetInitialWorkingDir(const boost::filesystem::path& dir);

            // システムコール群
            int GetErrno();

            int GetPID();
            int GetUID();
            int GetEUID();
            int GetGID();
            int GetEGID();

            char* GetCWD(char* buf, int maxlen);
            int ChDir(const char* path);

            // ファイルを開く．開いたファイルは自動でFDの変換表に追加される
            int Open(const char* filename,int oflag);

            int Read(int targetFD, void *buffer, unsigned int count);
            int Write(int targetFD, void* buffer, unsigned int count);
            int Close(int targetFD);

            int Stat(const char* path, HostStat* s);
            int FStat(int fd, HostStat* s);
            int LStat(const char* path, HostStat* s);

            s64 LSeek(int fd, s64 offset, int whence);
            int Dup(int fd);

            int Access(const char* path, int mode);
            int Unlink(const char* path);
            int Rename(const char* oldpath, const char* newpath);

            int Truncate(const char* path, s64 length);
            int FTruncate(int fd, s64 length);

            int MkDir(const char* path, int mode);

            // targetのFDとhostのFDの変換を行う
            int FDTargetToHost(int targetFD) const
            {
                return m_fdConv.TargetToHost(targetFD);
            }
            int FDHostToTarget(int hostFD) const
            {
                return m_fdConv.HostToTarget(hostFD);
            }

            // 時刻の取得
            s64 GetTime();
            s64 GetClock();
            void AddInsnTick()
            {
                m_executedInsnTick++;
            }
            s64 GetInsnTick()
            {
                return m_executedInsnTick;
            }

            DelayUnlinker* GetDelayUnlinker(){
                return &m_delayUnlinker;
            };

            BEGIN_PARAM_MAP("/Session/Emulator/System/Time/")
                PARAM_ENTRY( "@UnixTime",    m_unixTime )
                PARAM_ENTRY( "@EmulationMode", m_timeEmulationModeStr )
            END_PARAM_MAP()
        private:
            boost::filesystem::path GetHostPath(const char* targetPath);
            void AddAutoCloseFD(int fd);
            void RemoveAutoCloseFD(int fd);

            FDConv m_fdConv;
            // デストラクト時に自動でcloseするfd
            std::vector<int> m_autoCloseFD;
            DelayUnlinker m_delayUnlinker;

            boost::filesystem::path m_cwd;

            // 時刻
            int EmulationModeStrToInt( const std::string& );
            u64 m_unixTime;
            u64 m_executedInsnTick;
            std::string m_timeEmulationModeStr;
            int m_timeEmulationMode;
            enum {
                TIME_HOST,
                TIME_FIXED,
                TIME_INSTRUCTION,
            };
        };

    } // namespace EmulatorUtility
} // namespace Onikiri

#endif
