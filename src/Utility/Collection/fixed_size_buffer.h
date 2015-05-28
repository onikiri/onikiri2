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


#ifndef UTILITY_COLLECTION_FIXED_SIZE_BUFFER_H
#define UTILITY_COLLECTION_FIXED_SIZE_BUFFER_H

namespace Onikiri
{
    template < typename T, size_t SIZE, typename Tag = int >
    class fixed_sized_buffer : private boost::array< T, SIZE >
    {
    public:
        typedef typename boost::array< T, SIZE > base_type;

        typedef typename base_type::iterator        iterator;
        typedef typename base_type::const_iterator  const_iterator;
        typedef typename base_type::reference       reference;
        typedef typename base_type::const_reference const_reference;
        typedef typename base_type::size_type       size_type;

        fixed_sized_buffer() : 
            m_end( base_type::begin() + 0 )
        {

        }
                
        fixed_sized_buffer( const fixed_sized_buffer& rhs )
        {
            std::copy( rhs.begin(), rhs.end(), begin() );
            m_end = begin() + rhs.size();
        }

        fixed_sized_buffer& operator= ( const fixed_sized_buffer& rhs )
        {
            std::copy( rhs.begin(), rhs.end(), begin() );
            m_end = begin() + rhs.size();
            return *this;
        }

        // Same as boost::array.
        reference at( size_type i ) 
        { 
            return base_type::at( i );
        }

        const_reference at( size_type i ) const
        {
            return base_type::at( i );
        }

        reference operator[]( size_type i ) 
        { 
            return (*static_cast<base_type*>(this))[i];
        }
        
        const_reference operator[]( size_type i ) const 
        {     
            return (*static_cast<base_type*>(this))[i];
        }

        const T* data() const 
        {
            return base_type::data(); 
        }

        T* data() 
        {
            return base_type::data(); 
        }

        // Original
        size_type size() const
        {
            return m_end - begin();
        }

        void push_back( const T& value )
        {
            if( size() >= base_type::size() ){
                buffer_overflow();
            }

            *m_end = value;
            m_end++;
        }

        void pop_front()
        {
            erase( begin() );
        }
        
        iterator begin()
        {
            return base_type::begin();
        }

        iterator end()
        {
            return m_end;
        }

        const_iterator begin() const
        {
            return base_type::begin();
        }

        const_iterator end() const
        {
            return m_end;
        }

        void clear()
        {
            m_end = begin();
        }

        void resize( size_t size, const T& value = T() )
        {
            if( size >= base_type::size() ){
                buffer_overflow();
            }
            m_end = begin() + size;
            std::fill( begin(), end(), value );
        }
        
        iterator erase( const_iterator where )
        {
            const_iterator s = where + 1;
            iterator d = iterator( where );
            while( s != end() ){
                *d = *s;
                s++;
                d++;
            }
            //std::move< const_iterator, iterator >( where + 1, end(), iterator( where ) );
            m_end--;
            return iterator( where );
        }

    protected:
        iterator  m_end;

        NOINLINE void buffer_overflow()
        {
            THROW_RUNTIME_ERROR( 
                "A buffer is resized in excess of a max buffer size. "
                "The max buffer size (%d) is too small. "
                "Type tag: %s",
                (int)SIZE,
                typeid( Tag ).name()
            );
        }
    };
}

#endif
