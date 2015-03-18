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
// This file must be included after all switch macros relating to 
// BOOST are defined and before all BOOST header files are included.
//

#ifndef SYSDEPS_BOOST_H
#define SYSDEPS_BOOST_H

#include "SysDeps/host_type.h"

#ifdef COMPILER_IS_MSVC

	// The following files cause several warnings, and 
	// these warnings are suppressed by configuration switches of 
	// the .cpp files in the MSVC project file.
	//
	// - iostreams/mapped_file.cpp
	// - iostreams/zlib.cpp 
	// - system/error_code.cpp
	// - filesystem/path.cpp
	// - filesystem/path_traits.cpp
	// - filesystem/portability.cpp
	// - filesystem/utf8_codecvt_facet.cpp
	// - filesystem/windows_file_codecvt.cpp


	#pragma warning(push)
	
	// crc.hpp/gzip.hpp cause warnings 4245/4244 in MSVC.
	#pragma warning(disable:4244)
	#pragma warning(disable:4245)
	#include <boost/crc.hpp>
	#include <boost/iostreams/filter/gzip.hpp>

	// The following files cause warnings 4819 in MSVC.
	#pragma warning(disable:4819)
	#include <boost/array.hpp>
	#include <boost/scoped_array.hpp>
	#include <boost/lexical_cast.hpp>

	#pragma warning(pop)

#else

#endif

#endif

