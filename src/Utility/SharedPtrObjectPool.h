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


#ifndef __SHARED_PTR_OBJECT_POOL_H
#define __SHARED_PTR_OBJECT_POOL_H

namespace Onikiri
{

    template<typename T> 
    class SharedPtrObjectPool
    {
        struct SharedPtrObjectPoolTag
        {
        };

        typedef boost::singleton_pool<
            SharedPtrObjectPoolTag, 
            sizeof(T)
        > Pool;

        struct SharedPtrObjectPoolDeleter
        {
            void operator()(T* ptr)
            {
                ptr->~T();
                Pool::free(ptr);
            };
        };
    public:

        boost::shared_ptr<T> construct()
        {
            T* const ret = (T*)Pool::malloc();
            if(ret == 0)
                return boost::shared_ptr<T>( ret );
            try { new (ret) T(); }
            catch (...) { Pool::free(ret); throw; }

            return boost::shared_ptr<T>(
                    ret, 
                    SharedPtrObjectPoolDeleter() );
        };

        template <typename Arg0>
        boost::shared_ptr<T> construct(const Arg0& a0)
        {
            T* const ret = (T*)Pool::malloc();
            if(ret == 0)
                return boost::shared_ptr<T>( ret );
            try { new (ret) T(a0); }
            catch (...) { Pool::free(ret); throw; }

            return boost::shared_ptr<T>(
                    ret, 
                    SharedPtrObjectPoolDeleter() );
        };

        template <typename Arg0, typename Arg1>
        boost::shared_ptr<T> construct(const Arg0& a0, const Arg1& a1)
        {
            T* const ret = (T*)Pool::malloc();
            if(ret == 0)
                return boost::shared_ptr<T>( ret );
            try { new (ret) T(a0, a1); }
            catch (...) { Pool::free(ret); throw; }

            return boost::shared_ptr<T>(
                    ret, 
                    SharedPtrObjectPoolDeleter() );
        };

        template <typename Arg0, typename Arg1, typename Arg2>
        boost::shared_ptr<T> construct(const Arg0& a0, const Arg1& a1, const Arg2& a2)
        {
            T* const ret = (T*)Pool::malloc();
            if(ret == 0)
                return boost::shared_ptr<T>( ret );
            try { new (ret) T(a0, a1, a2); }
            catch (...) { Pool::free(ret); throw; }

            return boost::shared_ptr<T>(
                    ret, 
                    SharedPtrObjectPoolDeleter() );
        };

        template <typename Arg0, typename Arg1, typename Arg2, typename Arg3>
        boost::shared_ptr<T> construct(
            const Arg0& a0, const Arg1& a1, const Arg2& a2, const Arg3& a3)
        {
            T* const ret = (T*)Pool::malloc();
            if(ret == 0)
                return boost::shared_ptr<T>( ret );
            try { new (ret) T(a0, a1, a2, a3); }
            catch (...) { Pool::free(ret); throw; }

            return boost::shared_ptr<T>(
                    ret, 
                    SharedPtrObjectPoolDeleter() );
        };
    };

    template<typename T> 
    class PooledSharedPtrObject
    {
        static SharedPtrObjectPool<T>& GetPool()
        {
            static SharedPtrObjectPool<T> pool;
            return pool;
        }
    public:
        static boost::shared_ptr<T> Construct()
        {
            return GetPool().construct();
        };

        template <typename Arg0>
        static boost::shared_ptr<T> Construct(const Arg0& a0)
        {
            return GetPool().construct(a0);
        };

        template <typename Arg0, typename Arg1>
        static boost::shared_ptr<T> Construct(const Arg0& a0, const Arg1& a1)
        {
            return GetPool().construct(a0, a1);
        };

        template <typename Arg0, typename Arg1, typename Arg2>
        static boost::shared_ptr<T> Construct(const Arg0& a0, const Arg1& a1, const Arg2& a2)
        {
            return GetPool().construct(a0, a1, a2);
        };

        template <typename Arg0, typename Arg1, typename Arg2, typename Arg3>
        static boost::shared_ptr<T> Construct(
            const Arg0& a0, const Arg1& a1, const Arg2& a2, const Arg3& a3)
        {
            return GetPool().construct(a0, a1, a2, a3);
        };

    };  // template<typename T> class PooledSharedPtrObject

} //namespace Onikiri

#endif


