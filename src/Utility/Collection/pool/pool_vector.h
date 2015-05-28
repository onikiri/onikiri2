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
// STL vector wrapper for boost::fast_pool_allocator
//

#ifndef __POOL_VECTOR_H
#define __POOL_VECTOR_H

#include <vector>

#ifdef ONIKIRI_USE_ONIKIRI_POOL_ALLOCATOR
    #include "pool_allocator.h"
    #define ONIKIRI_POOL_VECTOR_ALLOCATOR pool_allocator
#else
    #include <boost/pool/pool_alloc.hpp>
    #define ONIKIRI_POOL_VECTOR_ALLOCATOR boost::pool_allocator
#endif

namespace Onikiri
{
    template <typename T>
    class pool_vector : 
        public std::vector<T, ONIKIRI_POOL_VECTOR_ALLOCATOR<T> >
    {
        typedef ONIKIRI_POOL_VECTOR_ALLOCATOR<T>   allocator_type;
        typedef std::vector<T, allocator_type > collection_type;

    public:
        pool_vector()
            : collection_type()
        {
        }

        explicit pool_vector(
            const allocator_type& allocator
        ) :
            collection_type(allocator)
        {
        }

        explicit pool_vector(
            size_t n, 
            const T& value = T(), 
            const allocator_type& allocator = allocator_type()
        ) : 
            collection_type(n, value, allocator) 
        {
        };

        template <class input_iterator>
        pool_vector(
            input_iterator begin, 
            input_iterator end, 
            const allocator_type& allocator = allocator_type()
        ) : 
            collection_type(begin, end, allocator)
        {
        };

        
        pool_vector(const pool_vector<T>& x)
            : collection_type(x)
        {
        }
    };

}; // namespace Onikiri

#endif
