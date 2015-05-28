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

#define BOOST_NO_MT
#define BOOST_DISABLE_THREADS
#define BOOST_SP_USE_QUICK_ALLOCATOR
#define BOOST_SP_DISABLE_THREADS
#define BOOST_ALL_NO_LIB
#define BOOST_SYSTEM_NO_LIB
#define NO_ZLIB 0

#ifndef ONIKIRI_DEBUG
#   define BOOST_DISABLE_ASSERTS
#endif


#ifdef COMPILER_IS_MSVC

#   pragma warning(push)

// Some boost files cause warnings 4244/4245/4267/4819 in MSVC 2013.
#   pragma warning(disable:4244)
#   pragma warning(disable:4245)
#   pragma warning(disable:4819)
#   pragma warning(disable:4267)

#elif defined COMPILER_IS_GCC

// push & pop is available since gcc 4.6
#   pragma GCC diagnostic push
    
#   pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#   pragma GCC diagnostic ignored "-Wunused-variable"

#elif defined COMPILER_IS_CLANG

// push & pop is available since gcc 4.6
#   pragma GCC diagnostic push

#   pragma GCC diagnostic ignored "-Woverloaded-virtual"

#endif



// This file is included at this point after enabling boost switches,
// because 'unordered_map' is implemented with boost currently.
#include "SysDeps/STL/unordered_map.h"

#include <boost/detail/quick_allocator.hpp>
#include <boost/pool/pool.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/pool/singleton_pool.hpp>
#include <boost/pool/pool_alloc.hpp>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/array.hpp>
#include <boost/scoped_array.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/crc.hpp>

#include <boost/filesystem.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>

#include <boost/crc.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/array.hpp>
#include <boost/scoped_array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/static_assert.hpp>


// #include <boost/compressed_pair.hpp>
// #include <boost/crc.hpp>
// #include <boost/any.hpp>
// #include <boost/multi_array.hpp>
// #include <boost/pool/detail/mutex.hpp>


#ifdef COMPILER_IS_MSVC
#   pragma warning(pop)
#elif defined COMPILER_IS_GCC
#   pragma GCC diagnostic pop
#elif defined COMPILER_IS_CLANG
#   pragma GCC diagnostic pop
#else



#endif


#endif

