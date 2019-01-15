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
            n = p.regex_replace(regex("([^/\\.]+)/\\.\\./"), "");
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
        return CanonicalizePath(IsAbsPath(path) ? path : String(base + "/" + path));
    }
}

void VirtualPath::SetVirtualRoot(const String& virtualRoot)
{
    m_virtualRoot = CanonicalizePath(virtualRoot);
}

void VirtualPath::SetHostRoot(const String& hostRoot)
{
    m_hostRoot = CanonicalizePath(hostRoot);
}

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

void VirtualPath::SetVirtualRelPath(const String& path)
{
    m_virtualPath = CanonicalizePath(path);
}

String VirtualPath::ToHost() const
{
    return CompletePath(m_virtualPath, m_hostRoot);
}

String VirtualPath::ToVirtual() const
{
    return CompletePath(m_virtualPath, m_virtualRoot);
}

