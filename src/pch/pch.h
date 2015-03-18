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
// Pre-compiled header
//

#ifndef PCH_PCH_H
#define PCH_PCH_H

//
// --- Environmentally dependent things ---
//
#include "SysDeps/host_type.h"
#include "SysDeps/Warning.h"


//
// --- Onikiri basic types --- 
//
#include "Types.h"
#include "Version.h"
#include "SysDeps/Inline.h"


//
// --- C General headers ---
//
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include "SysDeps/stdarg.h"


//
// --- C++/STL ---
//
#include <utility>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <fstream>
#include <exception>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <stack>

// for compatibility
#include "SysDeps/STL/list.h"

#include <algorithm>
#include <functional>

#include <cassert>
#include <cstddef>
#include <cmath>
#include <limits>
#include <iomanip>
#include <memory>


//
// --- Tiny XML ---
//
#include <tinyxml/tinyxml.h>


//
// --- Boost ---
//
#define BOOST_NO_MT
#define BOOST_DISABLE_THREADS
#define BOOST_SP_USE_QUICK_ALLOCATOR
#define BOOST_SP_DISABLE_THREADS
#define BOOST_ALL_NO_LIB
#define BOOST_SYSTEM_NO_LIB
#define NO_ZLIB 0

#ifndef ONIKIRI_DEBUG
#define BOOST_DISABLE_ASSERTS
#endif

#include "SysDeps/Boost/Boost.h"

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


// #include <boost/compressed_pair.hpp>
// #include <boost/crc.hpp>
// #include <boost/any.hpp>
// #include <boost/multi_array.hpp>
// #include <boost/pool/detail/mutex.hpp>



//
// --- Onikiri utility ---
//

// Use onikiri internal allocator because
// boost::pool_allocator has the bug that allocates infinite memory 
// when it is used with vector of vector.
#define ONIKIRI_USE_ONIKIRI_POOL_ALLOCATOR

#include "Utility/String.h"
#include "Utility/RuntimeError.h"
#include "Utility/SharedPtrObjectPool.h"
#include "Utility/IntrusivePtrObjectPool.h"

//#include "Utility/Collection/hashed_map_list.h"
//#include "Utility/Collection/hashed_index_list.h"

#include "Utility/Collection/pool/pool_list.h"
#include "Utility/Collection/pool/pool_vector.h"
#include "Utility/Collection/pool/pool_unordered_map.h"
#include "Utility/Collection/fixed_size_buffer.h"



//
// --- shttl ---
//
#define SHTTL_ASSERT(x) { using namespace Onikiri; ASSERT(x); }
#include "Lib/shttl/std_hasher.h"
#include "Lib/shttl/static_off_hasher.h"
#include "Lib/shttl/setassoc_table.h"
#include "Lib/shttl/null_struct.h"
#include "Lib/shttl/counter_array.h"
#include "Lib/shttl/table.h"

// #include <shttl/bit.h>
// #include <shttl/array2d.h>
// #include <shttl/null_data.h>
// #include <shttl/hasher.h>
// #include <shttl/lru.h>


//
// --- Onikiri interface ---
//
#include "Interface/Addr.h"
#include "Interface/EmulatorIF.h"
#include "Interface/ExtraOpDecoderIF.h"
#include "Interface/ISAInfo.h"
#include "Interface/MemIF.h"
#include "Interface/OpClass.h"
#include "Interface/OpInfo.h"
#include "Interface/OpStateIF.h"
#include "Interface/ResourceIF.h"
#include "Interface/SystemIF.h"

//
// --- Onikiri environment ---
//
#include "Env/Env.h"
#include "Env/Param/ParamPredResult.h"

//
// --- Additional user defined ---
//
#include "User/UserPCH.h"

#endif // #ifndef PCH_PCH_H
