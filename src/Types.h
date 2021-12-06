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


#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <cinttypes>

//
// basic data types
//

namespace Onikiri
{
    // int*_t is ALWAYS two's complement.

    using s64 = std::int64_t;
    using u64 = std::uint64_t;
    using s32 = std::int32_t;
    using u32 = std::uint32_t;
    using s16 = std::int16_t;
    using u16 = std::uint16_t;
    using s8  = std::int8_t;
    using u8  = std::uint8_t;

    using f32 = float;
    using f64 = double;
}

#endif  // #ifndef TYPES_H

