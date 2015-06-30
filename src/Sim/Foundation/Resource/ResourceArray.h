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


#ifndef __RESOURCE_ARRAY_H__
#define __RESOURCE_ARRAY_H__


#include "Types.h"

namespace Onikiri 
{
    template <class T>
    class PhysicalResourceArray
    {
        std::vector<T*> m_array;
    public:

        typedef T ValueType;
        typedef T* PtrType;

        PhysicalResourceArray()
        {
        }

        virtual ~PhysicalResourceArray()
        {
        }

        void Clear()
        {
            m_array.clear();
        }

        void Resize(int size)
        {
            m_array.resize( size );
        }

        int GetSize() const
        {
            return (int)m_array.size();
        }

        void Add(T* ptr)
        {
            m_array.push_back( ptr );
        }

        PtrType& At(int index) 
        {
            return m_array[index];
        }

        const PtrType& At(int index) const
        {
            return m_array[index];
        }


        PtrType& operator[] (int index) 
        {
            return m_array[index];
        }

        const PtrType&  operator[] (int index) const
        {
            return m_array[index];
        }
    };

}; // namespace Onikiri

#endif // __RESOURCE_BASE_H__

