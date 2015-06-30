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


#ifndef __INTRUISVE_PTR_OBJECT_POOL_H
#define __INTRUISVE_PTR_OBJECT_POOL_H
#include "Utility/Collection/pool/pool_allocator.h"
namespace Onikiri
{

    struct IntrusiveObjectPoolTag
    {
    };

    template<typename T, typename PtrT = T > 
    class PooledIntrusivePtrObject
    {
#if 1
        INLINE static T* Allocate()
        {
            typedef boost::singleton_pool<
                IntrusiveObjectPoolTag, 
                sizeof(T)
            > Pool;
            return (T*)Pool::malloc();
        }

        INLINE static void Deallocate(T* ptr)
        {
            typedef boost::singleton_pool<
                IntrusiveObjectPoolTag, 
                sizeof(T)
            > Pool;
            Pool::free(ptr);
        }
#else
        NOINLINE static T* Allocate()
        {
            typedef pool_allocator<T> Pool;
            return (T*)Pool().allocate(1);
        }

        INLINE static void Deallocate(T* ptr)
        {
            typedef pool_allocator<T> Pool;
            Pool().deallocate(ptr,1);
        }
#endif

    public:

        INLINE static void Destruct( T* ptr )
        {
            ptr->~T();
            Deallocate(ptr);
        }

        INLINE static boost::intrusive_ptr<PtrT> Construct()
        {
            T* const ret = Allocate();
            new (ret) T();
            return boost::intrusive_ptr<PtrT>(  ret );
        };

        template <typename Arg0>
        INLINE static boost::intrusive_ptr<PtrT> Construct(const Arg0& a0)
        {
            T* const ret = Allocate();
            new (ret) T(a0);
            return boost::intrusive_ptr<PtrT>(  ret );
        };

        template <typename Arg0, typename Arg1>
        INLINE static boost::intrusive_ptr<PtrT> Construct(const Arg0& a0, const Arg1& a1)
        {
            T* const ret = Allocate();
            new (ret) T(a0, a1);
            return boost::intrusive_ptr<PtrT>(  ret );
        };

        template <typename Arg0, typename Arg1, typename Arg2>
        INLINE static boost::intrusive_ptr<PtrT> Construct(const Arg0& a0, const Arg1& a1, const Arg2& a2)
        {
            T* const ret = Allocate();
            new (ret) T(a0, a1, a2);
            return boost::intrusive_ptr<PtrT>(  ret );
        };

        template <typename Arg0, typename Arg1, typename Arg2, typename Arg3>
        INLINE static boost::intrusive_ptr<PtrT> Construct(
            const Arg0& a0, const Arg1& a1, const Arg2& a2, const Arg3& a3)
        {
            T* const ret = Allocate();
            new (ret) T(a0, a1, a2, a3);
            return boost::intrusive_ptr<PtrT>(  ret );
        };


    };
} //namespace Onikiri

#endif  // __INTRUISVE_PTR_OBJECT_POOL_H


