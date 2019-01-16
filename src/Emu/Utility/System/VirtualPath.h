// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Masahiro Goshima.
// Copyright (c) 2005-2019 Ryota Shioya.
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

#ifndef EMU_UTILITY_VIRTUAL_PATH
#define EMU_UTILITY_VIRTUAL_PATH

#include "Utility/String.h"

namespace Onikiri {
namespace EmulatorUtility {

    // ファイルパスの仮想化を行うクラス
    // ホストの実際のパスを，Linux ベースのゲストのパスにマップする
    // たとえば "C:\test\data\test.txt" の "C:\test\data" を 
    // ゲストの "/ONIKIRI" にマップし，ゲスト側から "/ONIKIRI/test.txt"
    // でアクセスできるようにする
    class VirtualPath
    {
    public:
        VirtualPath()
        {
        }

        //void SetHostAbsPath(const String& path);

        // 仮想パスのルートを設定する
        // /ONIKIRI/test.txt
        void SetGuestRoot(const String& guestRoot);
        
        // ホスト側のルートを設定する
        void SetHostRoot(const String& hostRoot);

        // 仮想/ホスト のルートからの相対仮想パスを設定する
        void SetVirtualPath(const String& path);

        // 相対仮想パスを設定するの末尾にパスを追加する
        // 自動的に / が追加される
        void AppendVirtualPath(const String& path);

        // 現在の仮想パスをホスト/仮想側で表した場合のパスを返す
        String ToHost() const;
        String ToGuest() const;

        // 現在の仮想パス上での path の，ホストでのパスを返す
        String CompleteInHost(const String& path) const;

        // 現在の仮想パス上での path の仮想でのパスを返す
        String CompleteInGuest(const String& path) const;


    protected:
        String m_virtualPath;   // e.g. test.txt
        String m_guestRoot;     // e.g. /ONIKIRI
        String m_hostRoot;      // e.g. Z:\work
    };


}   // namespace EmulatorUtility {
}   // namespace Onikiri

#endif

