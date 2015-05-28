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


#ifndef SHTTL_ARRAY2D_H
#define SHTTL_ARRAY2D_H

#include <cmath>
#include <limits>
#include <stdexcept>
#include <iterator>
#include <memory>

#include "shttl_types.h"

namespace shttl 
{


    template<class T, class Allocator = std::allocator<T> >
    class array2d 
    {
    public:

        // Types

        typedef T            value_type;
        typedef size_t       size_type;
        typedef std::allocator<T> allocator_type;
        typedef T&           reference;
        typedef const T&     const_reference;
        typedef ssize_t      difference_type;
        typedef T*           pointer;
        typedef const T*     const_pointer;

        typedef array2d<T>   this_type;

        static const std::numeric_limits<size_type> size_info;


    protected:

        // iterators
        class _iterator_base 
        {
        protected:

            size_type  _r;
            size_type  _c;

            const size_type  _col;

            // Constructor
            _iterator_base(
                const size_type r, 
                const size_type c,
                const size_type col
            ) :
                _r(r), _c(c), _col(col)
            {
            }

        public:

            // Accessors

            size_type row() const  { return _r; }
            size_type col() const  { return _c; }

            // Comparing Operators

            bool operator==(const _iterator_base& rhs) const 
            {   return  _r == rhs._r && _c == rhs._c;   }
            bool operator!=(const _iterator_base& rhs) const 
            {   return !(*this == rhs);                 }
            bool operator<(const _iterator_base& rhs) const 
            {   return std::make_pair(_r, _c) <  std::make_pair(rhs._r, rhs._c);    }
            bool operator<=(const _iterator_base& rhs) const  
            {   return std::make_pair(_r, _c) <= std::make_pair(rhs._r, rhs._c);    }
            bool operator>(const _iterator_base& rhs) const  
            {   return std::make_pair(_r, _c) >  std::make_pair(rhs._r, rhs._c);    }
            bool operator>=(const _iterator_base& rhs) const  
            {   return std::make_pair(_r, _c) >= std::make_pair(rhs._r, rhs._c);    }

            _iterator_base& operator+=(const size_type c) 
            {
                const size_type cc = _c + c;
                if (cc < _col) {
                    _c = cc;
                } else {
                    _c  = cc % _col;
                    _r += cc / _col;
                }
                return *this;
            }

            _iterator_base& operator-=(const size_type c) 
            {
                const size_type cc = _c - c;
                if (cc > 0) {
                    _c = cc;
                } else {
                    _c  = cc % _col;
                    _r += cc / _col;
                }
                return *this;
            }
        };

    public:

        class const_iterator :
            public _iterator_base,
            public std::iterator<std::random_access_iterator_tag, T> 
        {
        private:

            const this_type* const _a;

            using _iterator_base::_r; 
            using _iterator_base::_c; 

        public:

            const_iterator(
                const this_type* const a, 
                const size_type r,
                const size_type c
            )  :
                _iterator_base(r, c, a->cols()),
                _a(a)
            {
            }

            bool operator==(const const_iterator& rhs) const  
            {
                return _iterator_base::operator==(rhs) && _a == rhs._a;
            }

            bool operator!=(const const_iterator& rhs) const  
            {
                return _iterator_base::operator!=(rhs) || _a != rhs._a;
            }

            const_iterator operator+(const size_type c) const 
            {
                const_iterator r = *this;
                return r._iterator_base.operator+=(c);
            }

            const_iterator operator-(const size_type c) const 
            {
                const_iterator r = *this;
                return r._iterator_base::operator-=(c);
            }

            const T& operator[](const size_type c) const
            {
                const_iterator i = *this + c;
                return *i;
            }
            /*
            operator const T*(){ 
                if (_r < 0 || _a->rows() < _r ||
                    _c < 0 || _a->cols() < _c
                ){
                    throw std::out_of_range("array2d::const_iterator::operator T*");
                }

                return &(_a->_ptr[(_r << _a->_col_bit) + _c]); 
            }
            */
            const_reference operator*()
            {
            #ifdef SHTTL_DEBUG
                if (_r < 0 || _a->rows() < _r ||
                    _c < 0 || _a->cols() < _c
                ){
                    throw std::out_of_range("array2d::const_iterator::operator T*");
                }
            #endif

                return _a->_ptr[(_r << _a->_col_bit) + _c]; 
            }
        };    


        class iterator :
            public _iterator_base,
            public std::iterator<std::random_access_iterator_tag, T> 
        {
        private:

            this_type* const _a;

            using _iterator_base::_r; 
            using _iterator_base::_c; 

        public:

            iterator(
                this_type* const a, 
                const size_type r, 
                const size_type c
            )  :
                _iterator_base(r, c, a->cols()),
                _a(a) 
            {
            }

            iterator operator+(const size_type c) const 
            {
                iterator r = *this;
                r._iterator_base::operator+=(c);
                return r;
            }

            iterator operator-(const size_type c) const 
            {
                iterator r = *this;
                r._iterator_base::operaotr-=(c);
                return r;
            }

            iterator& operator+=(const size_type c)  
            {
                this->_iterator_base::operator+=(c);
                return *this;
            }

            iterator& operator-=(const size_type c) 
            {
                this->_iterator_base::operator-=(c);
                return *this;
            }

            iterator& operator++() 
            {
                return *this += 1;
            }

            iterator& operator--() 
            {
                return *this -= 1;
            }

            iterator& operator++(int)  
            {
                iterator r = *this;
                *this += 1;
                return r;
            }

            iterator& operator--(int)  
            {
                iterator r = *this;
                *this -= 1;
                return r;
            }

            T& operator[](const size_type c) 
            {
                iterator i = *this + c;
                return *i;
            }      

            /*
            operator T*()
            { 
                if (_r < 0 || _a->rows() < _r ||
                    _c < 0 || _a->cols() < _c
                ){
                    throw stdf::out_of_range("array2d::const_iterator::operator T*");
                }

                return &(_a->_ptr[(_r << _a->_col_bit) + _c]); 
            }*/

            T& operator *()
            { 
            #ifdef SHTTL_DEBUG
                if (_r < 0 || _a->rows() < _r ||
                    _c < 0 || _a->cols() < _c
                ){
                    throw std::out_of_range("array2d::const_iterator::operator T*");
                }
            #endif

                return _a->_ptr[(_r << _a->_col_bit) + _c]; 
            }
        };

    protected:

        // Data Members

        const size_type _row;
        const size_type _col;
        const size_type _col_bit;

        value_type* const _ptr;

        allocator_type _a;

    public:

        // Accessors

        size_type  rows()      const  { return _row; }
        size_type  cols()      const  { return _col; }
        size_type  size()      const  { return rows() * cols(); }
        size_type  max_size()  const  { return size(); }


        // Constructors

    private:

        // c    | 1 2 3 4 5 6 7 8 ...
        // _c2b | 0 1 2 2 3 3 3 3 ...
        // log_2(c) | 0 1   2       3 ...

        size_type _c2b(const size_type c) const
        {
            if (c <= 0) 
                throw std::invalid_argument("array2d::_c2b");

            const double log2c = log(static_cast<double>(c)) / log(2.0);
            return static_cast<size_type>(ceil(log2c));
        }

    public:

        explicit array2d(
            const size_type c, 
            const size_type r, 
            const T& t = T()
        ) :
            _row(r), _col(c),
            _col_bit(_c2b(c)),
            _ptr(_a.allocate((size_t)r << _col_bit))
        {
            if (r <= 0 || c <= 0)
                throw std::invalid_argument("array2d::array2d");

            for (size_type i = 0; i < (r << _col_bit); ++i)
                _ptr[i] = t;
        }

        ~array2d()
        {
            for (size_type i = 0; i < (rows() << _col_bit); ++i)
                _a.destroy(_ptr + i);
            _a.deallocate(_ptr, (size_t)rows() << _col_bit);
        }

        // Reference

        const const_iterator operator[](const size_type r) const 
        {
        #ifdef SHTTL_DEBUG
            if (r < 0 || rows() <= r)
                throw std::out_of_range("array2d::operator[]");
        #endif

            return const_iterator(this, r, 0);
        }

        iterator operator[](const size_type r) 
        {
        #ifdef SHTTL_DEBUG
            if (r < 0 || rows() <= r)
                throw std::out_of_range("array2d::operator[]");
        #endif

            return iterator(this, r, 0);
        }


        // begin() & end()

        const_iterator begin() const  
        { 
            return const_iteraotr(this,      0, 0);
        }
        const_iterator end()   const  
        { 
            return const_iterator(this, size(), 0); 
        }
        iterator begin() 
        { 
            return iterator(this,      0, 0); 
        }
        iterator end()    
        { 
            return iterator(this, size(), 0); 
        }


        // NOT Supported.

        //     insert() {}
        //     erase() {}
        //     swap() {}
        //     empty() {}
        //     clear() {}

    }; // class array2d

} // namespace shttl

#endif // SHTTL_ARRAY2D_H
