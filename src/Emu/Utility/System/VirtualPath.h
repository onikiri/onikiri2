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

    class VirtualPath
    {
    public:
        VirtualPath()
        {
        }

        void SetHostAbsPath(const String& path);
        void SetVirtualRoot(const String& virtualRoot);
        void SetHostRoot(const String& hostRoot);
        void SetVirtualRelPath(const String& path);

        String ToHost() const;
        String ToVirtual() const;
    protected:
        String m_virtualPath;          // e.g. test.txt
        String m_virtualRoot;   // e.g. /ONIKIRI
        String m_hostRoot;      // e.g. Z:\work
    };


}   // namespace EmulatorUtility {
}   // namespace Onikiri

#endif

