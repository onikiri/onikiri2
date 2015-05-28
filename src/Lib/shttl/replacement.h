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


#ifndef SHTTL_RPLCMNT_H
#define SHTTL_RPLCMNT_H

#include "shttl_types.h"

namespace shttl 
{
    //
    // A base class of a replacer.
    // Each class that implements a replace algorithm inherits this class. 
    //
    // 'key_type' is a type of a key passed by 'setassoc_table' and 
    // is used for user implementation.
    //
    template < typename key_t >
    class replacer 
    {
    public:
        typedef size_t size_type;
        typedef key_t  key_type;
        static const size_type invalid_way = ~((size_type)0);

    #ifdef SHTTL_DEBUG
        // These pure virtual functions are only for type checking.

        virtual ~replacer() {}    

        // Constructs a replacer.
        virtual void construct(
            const size_type set_num,
            const size_type way_num
        ) = 0;

        virtual void touch(
            const size_type index,
            const size_type way,
            const key_type  key
        ) = 0;

        virtual size_type target(
            const size_type index
        ) = 0;

    #endif

    };

} // namespace shttl

#endif // SHTTL_RPLCMNT_H
