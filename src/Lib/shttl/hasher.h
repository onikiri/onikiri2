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


#ifndef SHTTL_HASHER_H
#define SHTTL_HASHER_H

#include <stdexcept>
#include "bit.h"

namespace shttl
{


    template<typename T>
    class hasher 
    {
    public:

        // Types
        typedef size_t size_type;

    #ifdef SHTTL_DEBUG
    
        virtual ~hasher() {}

        // Member Functions

        // indexÇÃêî([0, N-1]ÇÃN)Çï‘Ç∑
        virtual size_type size() const = 0;
        
        // index (Ç”Ç¬Ç§â∫à bit)
        virtual size_type index(const T& key) const = 0;
        
        // tag (Ç”Ç¬Ç§è„à bit)
        virtual T tag(const T& key) const = 0;
        
        // indexÇ∆tagÇ©ÇÁkeyÇïúå≥Ç∑ÇÈ
        virtual T rebuild(const T& tag, size_type index) const = 0;

        // lhsÇ∆rhsÇ™àÍívÇ∑ÇÈÇ©
        virtual bool match(const T& lhs, const T&rhs) const = 0;

    #endif

    };

} // namespace shttl

#endif // SHTTL_HASHER_H
