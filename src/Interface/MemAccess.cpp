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


#include <pch.h>
#include "Interface/MemAccess.h"

using namespace Onikiri;

//
// Returns a string that represents the current object.
// 
const std::string MemAccess::ToString() const
{
    // Convert the value to a string depending on its size.
    String strValue;
    for( int i = 0; i < 8; i++ ){
        if( i < size ){
            strValue = String().format( "%02x", (( value >> (i*8) ) & 0xff) ) + strValue;
        }
    }
    
    String str;
    str.format(
        "pid:%d tid:%d address:%08x%08x value:%s size:%d(%s)\n",
        address.pid,
        address.tid,
        (u32)(address.address >> 32), 
        (u32)(address.address & 0xffffffff),
        strValue.c_str(),
        size,
        sign ? "signed" : "unsigned"
    );  
    
    switch( result ){
    default:
        str += "The memory access is successful.";
        break;
    case MemAccess::MAR_READ_INVALID_ADDRESS:
        str += "MAR_READ_INVALID_ADDRESS: The memory was not assigned and could not be read.";
        break;
    case MemAccess::MAR_READ_NOT_READABLE:
        str += "MAR_READ_NOT_READABLE: The memory could not be read.";
        break;
    case MemAccess::MAR_READ_UNALIGNED_ADDRESS:
        str += "MAR_READ_UNALIGNED_ADDRESS: An misaligned read access occurs.";
        break;
    case MemAccess::MAR_WRITE_INVALID_ADDRESS:
        str += "MAR_WRITE_INVALID_ADDRESS: The memory was not assigned and could not be written.";
        break;
    case MemAccess::MAR_WRITE_UNALIGNED_ADDRESS:
        str += "MAR_WRITE_UNALIGNED_ADDRESS: An misaligned write access occurs.";
        break;
    case MemAccess::MAR_WRITE_NOT_WRITABLE:
        str += "MAR_WRITE_NOT_WRITABLE: The memory could not be written.";
        break;
    }

    return str;
}
