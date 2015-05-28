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


#ifndef SHTTL_XBITSET_H
#define SHTTL_XBITSET_H

#include <stdexcept>
#include <string>
#include <iostream>

#include "bit.h"
#include "bitset.h"


namespace shttl 
{

    template<int N>
    class xbitset : public shttl_bitset<N> 
    {
    public:

        // Types
        typedef xbitset<N>      this_type;
        typedef shttl_bitset<N> base_type;
        typedef ssize_t         size_type;

        // Accessors
        using base_type::size;
        using base_type::to_ulong;

        // Constructors
        xbitset(const u64 v = 0) : base_type(v)
        {
        }

        template<class C, class R, class A>
        explicit xbitset(
            const    std::basic_string<C, R, A>& s,
            typename std::basic_string<C, R, A>::size_type p = 0,
            typename std::basic_string<C, R, A>::size_type l = -1
        ) :
            base_type(s, p, l)
        {
        }

        xbitset(const xbitset& rhs) : base_type(rhs) 
        {
        }

        xbitset& operator=(const xbitset& rhs) 
        {
            base_type::operator=(rhs);
            return *this;
        }


        // CONST bit reference 
        class const_reference {

        public:

            operator bool()  const { return  _i.test(_p); }
            bool operator~() const { return !_i.test(_p); }

        private:

            const this_type& _i;
            const size_type  _p;

            const_reference(const this_type& i, const size_type p) :
                _i(i),
                _p(p)
            {
            }

        };

        class reference : public base_type::reference {

        public:

            reference& operator=(const const_reference& r) 
            {
                _i.set(_p, r.operator bool());
                return *this;
            }

        private:

            using base_type::reference::_i;
            using base_type::reference::_p;
        };


        // Const Element Access
        const const_reference operator[](const size_type p) const 
        {
            return const_reference(this, p);
        }

        const const_reference at(const size_type p) const
        { 
            if (p < 0 || size() <= p){
                throw std::out_of_range("xbitset::at");
            }

            return operator[](p);
        }

        // all

        bool all() const  
        { 
            for (int p = 0; p < N; ++p)
                if (!base_type::test(p))
                    return false;
            return true;
        }

        bool none() const  
        { 
            for (int p = 0; p < N; ++p)
                if (base_type::test(p))
                    return false;
            return true;
        }

        bool any() const  
        { 
            for (int p = 0; p < N; ++p)
                if (base_type::test(p))
                    return true;
            return false;
        }

        bool any(  const bool v ) const  { return v ? any()  : !all(); }
        bool none( const bool v ) const  { return v ? none() :  all(); }
        bool all(  const bool v ) const  { return v ? all()  : none(); }



    public:

        static const size_type npos = -1;

        size_type 
            find(const int v = 0, const size_type s = 0) const 
        {
        #ifdef SHTTL_DEBUG
            if (s < 0 || size() <= s){
                throw std::out_of_range("xbitset::find");
            }
        #endif

            if (v == 0 && s == 0 && size() <= 4) 
                return _find_0_map[to_ulong()];        
            else 
                return _find(v, s);
        } 

        size_type 
            find_and_flip(const int v = 0, const size_type s = 0) 
        {
        #ifdef SHTTL_DEBUG
            if (s < 0 || size() <= s){
                throw std::out_of_range("xbitset::find_and_flip");
            }
        #endif

            const size_type p = find(v, s);
            if (p != npos)
                base_type::flip(p);
            return p;
        }


        // Increment & Decrement 
        void reset(void) {
            *this = 0;
        }

        this_type& operator++()
        {
            u64 v = to_ulong();
            if (v < umax(size()))
                *this = v + 1;
            return *this;
        }

        this_type& operator--()
        {
            u64 v = to_ulong();
            if (v > umin(size()))
                *this = v - 1;
            return *this;
        }

        u64 operator++(int)
        {
            u64 v = to_ulong();
            if (v < umax(size()))
                *this = v + 1;
            return v;
        }

        u64 operator--(int)
        {
            u64 v = to_ulong();
            if (v > umin(size()))
                *this = v - 1;
            return v;
        }

        this_type& inc()
        {
            u64 v = to_ulong();
            if (v < umax(size())) 
                *this = v + 1;
            else
                throw std::overflow_error("xbitset::inc");
            return *this;
        }

        this_type& dec()
        {
            u64 v = to_ulong();
            if (v > umin(size()))
                *this = v - 1;
            else
                throw std::underflow_error("xbitset::dec");
            return *this;
        }

        // Find

    protected:

        size_type _find(const int v, const size_type s) const ;

        static const signed char _find_0_map[16];

    }; // class xbitset


    // Non-inline Member Functions


    template<int N>
    typename xbitset<N>::size_type 
        xbitset<N>::_find(const int v, const typename xbitset<N>::size_type s) const 
        
    {
        const bool b = v != 0;
        for (size_type p = s; p < size(); ++p) 
            if (base_type::test(p) == b)
                return p;
        return npos;
    }


    // Map to speed-up find(0)

    template<int N>
    const signed char xbitset<N>::_find_0_map[16] = {
        -1
    };

    template<>
    const signed char xbitset<1>::_find_0_map[16] = {
        0, // 0
        -1  // 1
    };

    template<>
    const signed char xbitset<2>::_find_0_map[16] = {  
        0, // 00
        1, // 01
        0, // 10
        -1  // 11
    };

    template<>
    const signed char xbitset<3>::_find_0_map[16] = {
        0, // 000
        1, // 001
        0, // 010
        2, // 011
        0, // 100
        1, // 101
        0, // 110
        -1  // 111
    };

    template<>
    const signed char xbitset<4>::_find_0_map[16] = {
        0, // 0000
        1, // 0001
        0, // 0010
        2, // 0011
        0, // 0100
        1, // 0101
        0, // 0110
        3, // 0111
        0, // 1000
        1, // 1001
        0, // 1010
        2, // 1011
        0, // 1100
        1, // 1101
        0, // 1110
        -1  // 1111
    };

} // namespace shttl

#endif // SHTTL_XBITSET_H
