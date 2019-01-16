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

#include "pch.h"
#include "Emu/Utility/System/VirtualPath.h"

using namespace std;
using namespace Onikiri;
using namespace EmulatorUtility;

// Canonicalize a path
namespace
{
    String CanonicalizePath(const String& path)
    {
        String p = path;
        p = p.regex_replace(regex("\\\\"), "/");

        String n = p;
        do {
            p = n;
            n = p.regex_replace(regex("[^/\\.]+/\\.\\."), "");
            n = n.regex_replace(regex("//"), "/");
            n = n.regex_replace(regex("/\\./"), "/");
            n = n.regex_replace(regex("/$"), "");
        } while (n != p);
        return p;
    };

    bool IsAbsPath(const String& path)
    {
        return path.at(0) == '/';
    }

    String CompletePath(const String& path, const String& base)
    {
        return IsAbsPath(path) ? path : String(base + "/" + path);
    }
}

void VirtualPath::SetGuestRoot(const String& guestRoot)
{
    m_guestRoot = CanonicalizePath(guestRoot);
}

void VirtualPath::SetHostRoot(const String& hostRoot)
{
    m_hostRoot = CanonicalizePath(hostRoot);
}

/*
// path から m_hostRoot を除いたパスを virtual path として設定
void VirtualPath::SetHostAbsPath(const String& path)
{
    String cPath = CanonicalizePath(path);
    regex p = regex(string("^") + m_hostRoot);
    if (cPath.regex_search(p)) {
        m_virtualPath = cPath.regex_replace(p, "");
    }
    else {
        THROW_RUNTIME_ERROR(
            "Specified path (%s) does not have a host root(%s)", 
            path.c_str(), 
            m_hostRoot.c_str()
        );
    }
}
*/

void VirtualPath::SetVirtualPath(const String& path)
{
    m_virtualPath = CanonicalizePath(path);
}

void VirtualPath::AppendVirtualPath(const String& path)
{
    m_virtualPath = CanonicalizePath(m_virtualPath + "/" + path);
}

String VirtualPath::ToHost() const
{
    return CompletePath(m_virtualPath, m_hostRoot);
}

String VirtualPath::CompleteInHost(const String& path) const
{
    String cPath = CanonicalizePath(CompletePath(path, m_virtualPath));
    regex p = regex(string("^") + m_guestRoot);
    if (cPath.regex_search(p)) {
        cPath = cPath.regex_replace(p, m_hostRoot + "/");
    }
    return CompletePath(cPath, m_hostRoot);
}

String VirtualPath::ToGuest() const
{
    return CompletePath(m_virtualPath, m_guestRoot);
}

String VirtualPath::CompleteInGuest(const String& path) const
{
    String cPath = CanonicalizePath(CompletePath(path, m_virtualPath));
    regex p = regex(string("^") + m_guestRoot);
    if (cPath.regex_search(p)) {
        return path;
    }
    else {
        return CompletePath(path, m_guestRoot);
    }
}

