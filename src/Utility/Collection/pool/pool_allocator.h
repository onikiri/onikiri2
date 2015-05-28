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
// Pool allocator which is compatible with STL collection class.
// This allocator never free memory and corrupt in multi-thread.
//

#ifndef __ONIKIRI_POOL_H
#define __ONIKIRI_POOL_H

#include <stack>
#include <vector>

//#define ONIKIRI_POOL_ALLOCATOR_INVESTIGATE

namespace Onikiri
{
    static const int POOL_ALLOCATOR_CHUNK_SIZE_BASE = 64;

    template <typename T>
    class pool_body
    {
    public:
        typedef T                  value_type;
        typedef value_type*        pointer;
        typedef value_type&        reference;
        typedef const value_type*  const_pointer;
        typedef const value_type&  const_reference;
        typedef size_t             size_type;
        typedef ptrdiff_t          difference_type;

    protected:  

        template <typename U>
        class pool_stack
        {
            std::vector<U> stack_body;
            size_t stack_top;
        public:

            pool_stack() : 
                stack_body(POOL_ALLOCATOR_CHUNK_SIZE_BASE),
                stack_top(0)
            {
            };

            void push(const U& v)
            {
                if(stack_body.size() <= stack_top){
                    stack_body.resize(stack_body.size()*2);
                }
                stack_body[stack_top] = v;
                stack_top++;
            }

            void pop()
            {
                stack_top--;
            }

            U& top()
            {
                assert(stack_top != 0);
                return stack_body[stack_top - 1];
            }

            size_t size()
            {
                return stack_top;
            }
        };

        //typedef std::stack<T*>   stack_type;
        typedef pool_stack<T*>     stack_type;
        std::vector< stack_type* > ptr_stack_array;
        std::vector<T*> ptr_list;

    #ifdef ONIKIRI_POOL_ALLOCATOR_INVESTIGATE
        size_t total_memory;
        size_t total_count;
        std::vector<size_t> total_memory_array;
        std::vector<size_t> total_count_array;
    #endif
        void allocate_chank( size_type count, stack_type* stack )
        {
            size_t chunk_size = POOL_ALLOCATOR_CHUNK_SIZE_BASE / count;
            if(chunk_size < 4)
                chunk_size = 4;

            // Throw std::bad_alloc if memory allocation failed.
            pointer chunk = (pointer)( ::operator new(chunk_size * count * sizeof(T)) );
            ptr_list.push_back(chunk);

            for( size_t i = 0; i < chunk_size; i++){
                stack->push( chunk );
                chunk += count;
            }

        #ifdef ONIKIRI_POOL_ALLOCATOR_INVESTIGATE
            total_memory += chunk_size * count * sizeof(T);
            total_count  += chunk_size * count;
            if( total_memory_array.size() <= count ){
                total_memory_array.resize( count + 1 );
                total_count_array.resize( count + 1 );
            }
            total_memory_array[count] += chunk_size * count * sizeof(T);
            total_count_array[count] += chunk_size * count;
        #endif
        }

    public:

        pool_body()
        {
        };

        ~pool_body()
        {
            for(size_t i = 0; i < ptr_list.size(); i++){
                ::operator delete( (void*)ptr_list[i] );
            }
            for(size_t i = 0; i < ptr_stack_array.size(); i++){
                if(ptr_stack_array[i]){
                    delete ptr_stack_array[i];
                }
            }
        #ifdef ONIKIRI_POOL_ALLOCATOR_INVESTIGATE
            printf( "pool_allocator< %s > : \n\ttype size : %d bytes\n\ttotal : %d bytes, %d elements\n", typeid(T).name(), (int)sizeof(T), (int)total_memory, (int)total_count );
            for( size_t i = 0; i < total_memory_array.size(); i++ ){
                if( total_memory_array[i] > 0)
                    printf("\t%d : %d bytes, %d elements\n", (int)i, (int)total_memory_array[i], (int)total_count_array[i] );
            }
        #endif
        };

        INLINE pointer allocate(
            size_type count, 
            const void* hint = 0)
        {
            if(count == 0)
                return 0;

            if(ptr_stack_array.size() <= count){
                ptr_stack_array.resize(count+1, 0);
            }

            stack_type* stack = ptr_stack_array[count];
            if(stack == 0){
                // Throw std::bad_alloc if memory allocation failed.
                stack = new stack_type();
                ptr_stack_array[count] = stack;
            }
            
            // Allocate memory chunk
            if(stack->size() == 0){
                allocate_chank( count, stack );
            }

            pointer ptr = stack->top();
            stack->pop();
            return ptr;
        }

        INLINE void deallocate(pointer ptr, size_type count)
        {
            stack_type* stack = ptr_stack_array[count];
            stack->push(ptr);
        }   
    };

    template <typename T>
    class pool_allocator
    {
        typedef pool_body<T> pool_type;
        INLINE pool_type& pool()
        {
            static pool_type p;
            return p;
        }

    public:

        typedef T                  value_type;
        typedef value_type*        pointer;
        typedef value_type&        reference;
        typedef const value_type*  const_pointer;
        typedef const value_type&  const_reference;
        typedef size_t             size_type;
        typedef ptrdiff_t          difference_type;

        template <class U>
        struct rebind
        {
            typedef pool_allocator<U> other;
        };

        pool_allocator()
        {
        }

        pool_allocator(const pool_allocator&)
        {
        }

        template <class U> 
        pool_allocator(const pool_allocator<U>&)
        {
        }

        ~pool_allocator()
        {
        }

        INLINE pointer allocate(
            size_type count, 
            const void* hint = 0)
        {
            return pool().allocate(count);

        }

        INLINE void construct(pointer ptr, const T& value)
        {
            new (ptr) T(value);
        }

        INLINE void deallocate(pointer ptr, size_type count)
        {
            return pool().deallocate(ptr, count);
        }

        INLINE void destroy(pointer ptr)
        {
            ptr->~T();
        }

        pointer address(reference value) const 
        {
            return &value; 
        }

        const_pointer address(const_reference value) const 
        {
            return &value; 
        }

        size_type max_size() const
        {
            return std::numeric_limits<size_t>::max() / sizeof(T);
        }
    };


    template <class T1, class T2>
    bool operator==(const pool_allocator<T1>&, const pool_allocator<T2>&) 
    {
        return true; 
    }

    template <class T1, class T2>
    bool operator!=(const pool_allocator<T1>&, const pool_allocator<T2>&) 
    {
        return false; 
    }

};  // namespace Onikiri

#endif

