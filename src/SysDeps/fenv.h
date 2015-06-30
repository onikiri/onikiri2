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


#ifndef __SYSDEPS_FPU_H__
#define __SYSDEPS_FPU_H__

// C99 fp environment functions

#include "SysDeps/host_type.h"

#if defined( COMPILER_IS_MSVC )
#include <float.h>

const int FE_DOWNWARD = _RC_DOWN;
const int FE_UPWARD = _RC_UP;
const int FE_TONEAREST = _RC_NEAR;
const int FE_TOWARDZERO = _RC_CHOP;

inline int fegetround(void)
{
    unsigned int fpcr;
    _controlfp_s(&fpcr, 0, 0);
    return (int)(fpcr & _MCW_RC);
}

inline int fesetround(int rounding_mode)
{
    unsigned int fpcr;
    return _controlfp_s(&fpcr, (unsigned int)rounding_mode, _MCW_RC);
}


#elif defined( COMPILER_IS_GCC ) || defined(COMPILER_IS_CLANG)
#   ifndef _GNU_SOURCE
#       define _GNU_SOURCE
#   endif
#   include <fenv.h>
#else
#   error "unknown compiler"
#endif

const int FE_ROUNDDEFAULT = FE_TONEAREST;

namespace Onikiri {
    // setroundÇñàâÒçsÇ§Ç±Ç∆Ç…ÇÊÇÈê´î\í·â∫ÇÕÇŸÇ∆ÇÒÇ«Ç»Ç¢
    class ScopedFESetRound {
    public:
        ScopedFESetRound(int mode) {
            m_oldMode = fegetround();
            fesetround(mode);
        }
        ~ScopedFESetRound() {
            fesetround(m_oldMode);
        }
    private:
        int m_oldMode;
    };
}

#endif
