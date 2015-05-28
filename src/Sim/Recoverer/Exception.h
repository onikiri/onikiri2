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
// Definition of exception 
//

#ifndef SIM_RECOVERER_EXCEPTION_H
#define SIM_RECOVERER_EXCEPTION_H


namespace Onikiri
{

    struct Exception
    {
        // The type of exception
        enum ExceptionType
        {
            // Invalid (exception does not occur)
            ET_INVALID,

            // Division by zero (not implemented yet)
            ET_DIVISION_BY_ZERO,

            // User defined.
            ET_USER_0,
            ET_USER_1,
            ET_USER_2,
            ET_USER_3
        };

        bool exception;
        ExceptionType type;

        Exception() : 
            exception( false ),
            type( ET_INVALID )
        {
        }
    };
}

#endif

