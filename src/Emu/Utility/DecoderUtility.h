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


#ifndef __EMULATORUTILITY_DECODERUTILITY_H__
#define __EMULATORUTILITY_DECODERUTILITY_H__

namespace Onikiri {
    namespace EmulatorUtility {

        // begin ビット目 (LSB=0) からbeginビットを含めてlen bitだけ取り出す
        // sextがtrueのとき符号拡張を行う
        template <typename T>
        T ExtractBits(T value, int begin, int len, bool sext = false)
        {
            T result = (value >> begin) & ~(~(T)0 << len);
            // sign extend
            if (sext && (result & ((T)1 << (len-1))))
                result |= ~0 << len;
            return result;
        }

    } // namespace PPC64Linux
} // namespace Onikiri

#endif
