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
// STL list wrapper for pool_allocator
// This class manages its size for speed.
// ( A size() functin of libstdc++ consumes O(n).
//

#ifndef __POOL_LIST_H
#define __POOL_LIST_H

#include <list>

#ifdef ONIKIRI_USE_ONIKIRI_POOL_ALLOCATOR
    #include "pool_allocator.h"
    #define ONIKIRI_POOL_LIST_ALLOCATOR pool_allocator
#else
    #include <boost/pool/pool_alloc.hpp>
    #define ONIKIRI_POOL_LIST_ALLOCATOR boost::fast_pool_allocator
#endif

namespace Onikiri
{
    template <typename T>
    class pool_list :
        public std::list<T, ONIKIRI_POOL_LIST_ALLOCATOR<T> >
    {
        typedef ONIKIRI_POOL_LIST_ALLOCATOR<T> allocator_type;
        typedef std::list<T, allocator_type >  base_type;

    public:

        pool_list()
            : base_type()
        {
        }

        explicit pool_list(
            const allocator_type& allocator
        ) :
            base_type(allocator)
        {
        }

        explicit pool_list(
            size_t n, 
            const T& value = T(), 
            const allocator_type& allocator = allocator_type()
        ) : 
            base_type(n, value, allocator) 
        {
        };

        template <class input_iterator>
        pool_list(
            input_iterator begin, 
            input_iterator end, 
            const allocator_type& allocator = allocator_type()
        ) : 
            base_type(begin, end, allocator)
        {
        };

        pool_list(const pool_list<T>& x)
            : base_type(x)
        {
        }
    };

}; // namespace Onikiri

#endif
