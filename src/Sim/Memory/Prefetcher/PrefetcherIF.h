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


#ifndef SIM_MEMORY_PREFETCHER_PREFETCHER_IF_H
#define SIM_MEMORY_PREFETCHER_PREFETCHER_IF_H


namespace Onikiri
{
    class Cache;
    struct CacheHookParam;

    class PrefetcherIF
    {
    public:
        
        virtual ~PrefetcherIF(){};

        // Cache read
        // This method is called by all cache read accesses.
        virtual void OnCacheRead( Cache* cache, CacheHookParam* param ) = 0;

        // Cache write
        // This method is called by all cache write accesses.
        virtual void OnCacheWrite( Cache* cache, CacheHookParam* param ) = 0;

        // Invalidate line
        virtual void OnCacheInvalidation( Cache* cache, CacheHookParam* param ) = 0;

        // Write an access to the cache table.
        virtual void OnCacheTableUpdate( Cache* cache, CacheHookParam* param ) = 0;
    };


}; // namespace Onikiri

#endif // SIM_MEMORY_PREFETCHER_PREFETCHER_IF_H

