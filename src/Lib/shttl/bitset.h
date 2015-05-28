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
// STL compatible light-weight bitset 
//

#ifndef SHTTL_BITSET_H
#define SHTTL_BITSET_H

#include <bitset>
#include <string>
#include <stdexcept>
#include <cassert>
#include <climits>

#include "bit.h"

namespace shttl
{

    //
    //  shttl_bitset<N, T> : bitset<N> only for smaller N's 
    //

    template<size_t N, class T = u64>
    class shttl_bitset {

        friend class reference;

    public:

        typedef T                   value_type;
        typedef shttl_bitset<N, T>  this_type;
        typedef size_t              size_type;

    protected:

        // Value

        value_type _v;

    public:

        // Constructors

        shttl_bitset(u64 v = 0) :
          _v( (value_type)( v & umax(size()) ) )
        {
        }

        template<class C, class R, class A>
        explicit
            shttl_bitset(
                const    std::basic_string<C, R, A>& s,
                typename std::basic_string<C, R, A>::size_type p = 0,
                typename std::basic_string<C, R, A>::size_type l = -1
            )
        {
            std::bitset<u64_bits> bs;
            try {
                bs = std::bitset<u64_bits>(s, p, l);
            }
            catch (std::invalid_argument) {
                throw std::invalid_argument("shttl_bitset::shttl_bitset");
            }
            _v = (value_type)( bs.to_ulong() & umax(size()) );
        }

        // bit reference 

        class reference {
        protected:

            this_type& _i;
            const size_type  _p;

            reference(this_type& i, size_type p) : _i(i), _p(p) {}

        public:

            operator bool()  const { return  _i.test(_p); }
            bool operator~() const { return !_i.test(_p); }

            reference& operator=(bool v) {
                _i.set(_p, v);
                return *this;
            }

            reference& operator=(const reference& r) {
                _i.set(_p, r.operator bool());
                return *this;
            }

            reference& flip() {
                _i.flip(_p);
                return *this;
            }

        };

        // Bitwise Operators and Bitwise Operator Assignment

        this_type& operator&=(const this_type& i) { _v &= i._v;   return *this;  }
        this_type& operator|=(const this_type& i) { _v |= i._v;   return *this;  }
        this_type& operator^=(const this_type& i) { _v ^= i._v;   return *this;  }

        this_type& operator<<=(size_type w) { 
            _v <<= w;
            _v &= umax(size());  
            return *this; 
        }
        this_type& operator>>=(size_type w) { 
            _v >>= w;
            _v &= umax(size());  
            return *this;  
        }


        // Set, Reset, Flip

        this_type& set() {
            _v = umax(size());
            return *this;
        }

        this_type& set(size_type p, int v = 1) {
            if (p < 0 || size() <= p)
                throw std::out_of_range("shttl_bitset<N, T>::set");
            if (v) 
                _v |=  mask(p);
            else
                _v &= ~mask(p);
            return *this;
        }

        this_type& reset() {
            _v = 0;
            return *this;
        }

        this_type& reset(size_type p) {
            if (p < 0 || size() <= p)
                throw std::out_of_range("shttl_bitset<N, T>::reset");
            _v &= ~mask(p);
            return *this;
        }

        this_type operator~() const {
            return this_type(~_v & umax(size()));
        }

        this_type& flip() {
            _v = ~_v & umax(size());
            return *this;
        }

        this_type& flip(size_type p) {
            if (p < 0 || size() <= p)
                throw std::out_of_range("shttl_bitset<N, T>::flip");
            _v ^= mask(p);
            return *this;
        }


        // Element Access

        reference operator[](size_type p) { 
            SHTTL_ASSERT(0 <= p || p < size());
            return reference(*this, p); 
        }

        reference at(size_type p)
        { 
            if (p < 0 || size() <= p)
                throw std::out_of_range("shttl_bitset<N, T>::at");
            return reference(*this, p); 
        }

        u64 to_ulong() const {
            SHTTL_ASSERT(_v <= umax(size()));
            return _v;
        }

        std::string to_string() const;

        size_t count() const;
        size_t size() const { return N; }

        bool operator==(const this_type& i) const { return _v == i._v; }
        bool operator!=(const this_type& i) const { return _v != i._v; }

        bool test(size_type p) const {
            if (p < 0 || size() <= p)
                throw std::out_of_range("shttl_bitset::test");
            return shttl::test(_v, p);
        }

        bool any()  const { return _v != 0; }
        bool none() const { return _v == 0; }

        this_type operator<<(size_type s) const { 
            return this_type((_v << s) & umax(size()));
        }
        this_type operator>>(size_type s) const {
            return this_type((_v >> s) & umax(size()));
        }

    }; // class shttl_bitset


    // Non-member Operators

    template<size_t N, class T>
    inline
        shttl_bitset<N, T> 
        operator&(const shttl_bitset<N, T>& l, const shttl_bitset<N, T>& r) {
            return shttl_bitset<N, T>(l.to_ulong() & r.to_ulong());
    }

    template<size_t N, class T>
    inline
        shttl_bitset<N, T> 
        operator|(const shttl_bitset<N, T>& l, const shttl_bitset<N, T>& r) {
            return shttl_bitset<N, T>(l.to_ulong() | r.to_ulong());
    }

    template<size_t N, class T>
    inline
        shttl_bitset<N, T> 
        operator^(const shttl_bitset<N, T>& l, const shttl_bitset<N, T>& r) {
            return shttl_bitset<N, T>(l.to_ulong() ^ r.to_ulong());
    }

    template<size_t N, class T>
    inline
        std::istream& operator>>(std::istream& is, shttl_bitset<N, T>& i) {
            std::string s;
            is >> s;
            i = shttl_bitset<N, T>(s);
            return is;
    }

    template<size_t N, class T>
    inline
        std::ostream& operator<<(std::ostream& os, const shttl_bitset<N, T>& i) {
            std::string s;
            s = i.to_string();
            os << s;
            return os;
    }


    // Non-inline Member Fucntions
    template<size_t N, class T>
    std::string
        shttl_bitset<N, T>::to_string() const 
    {
        std::string s;
        for (int p = (int)size() - 1; p >= 0; --p) 
            s += test(p) ? '1' : '0';
        return s;
    }

    template<size_t N, class T>
    size_t 
        shttl_bitset<N, T>::count() const 
    {
        int s = 0;
        for (int p = 0; p < size(); ++p) 
            if (test(p))
                ++s;
        return s;
    }

}; // namespace shttl;


#endif // SHTTL_BITSET_H
