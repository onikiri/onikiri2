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
// 'unordered_map' defined in this file is not necessary
// if formal(C++0x) unordered_map is defined.
//

#ifndef __ONIKIRI_SYSDEPS_STL_UNORDERED_MAP_H
#define __ONIKIRI_SYSDEPS_STL_UNORDERED_MAP_H

#include <boost/tr1/unordered_map.hpp>

namespace Onikiri
{

#ifdef BOOST_HAS_TR1_UNORDERED_MAP
    using ::std::tr1::unordered_map;
    using ::std::tr1::hash;
#else
    using ::boost::unordered_map;
    using ::boost::hash;
#endif

}

#endif
