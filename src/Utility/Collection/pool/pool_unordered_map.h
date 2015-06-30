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
// STL unordered_map wrapper for boost::fast_pool_allocator
//

#ifndef __POOL_UNORDERED_MAP_H
#define __POOL_UNORDERED_MAP_H

#include "SysDeps/STL/unordered_map.h"

#ifdef ONIKIRI_USE_ONIKIRI_POOL_ALLOCATOR
    #include "pool_allocator.h"
    #define ONIKIRI_POOL_UNORDERED_MAP_ALLOCATOR pool_allocator
#else
    #include <boost/pool/pool_alloc.hpp>
    #define ONIKIRI_POOL_UNORDERED_MAP_ALLOCATOR boost::fast_pool_allocator
#endif

namespace Onikiri
{

    template <
        typename KeyT, 
        typename T,
        typename HashT,
        typename CmpT = std::equal_to<KeyT> 
    >
    class pool_unordered_map :
        public unordered_map<
            KeyT, T, HashT, CmpT,
            ONIKIRI_POOL_UNORDERED_MAP_ALLOCATOR<
                std::pair<const KeyT, T>
            > 
        >
    {
        typedef 
            ONIKIRI_POOL_UNORDERED_MAP_ALLOCATOR< 
                std::pair<const KeyT, T>
            > 
            allocator_type;

        typedef 
            unordered_map<
                KeyT, T, HashT, CmpT, 
                allocator_type 
            >
            collection_type;
            
        
    public:
        pool_unordered_map()
            : collection_type()
        {
        }

        explicit pool_unordered_map(
            size_t initial_bucket_count,
            const HashT& hash = HashT()
        )
            : collection_type( initial_bucket_count, hash )
        {
        }

        pool_unordered_map(
            const pool_unordered_map<KeyT, T, HashT, CmpT>& x
        )
            : collection_type(x)
        {
        }
    };
}

#endif
