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
// - PhysicalResourceIF
// An interface of physical resources.
// Ex. registers, exec units and branch predictors.
//
// - LogicalResourceIF
// An interface of logical resources.
// Ex. op and address.
//

#ifndef __RESOURCE_IF_H__
#define __RESOURCE_IF_H__


#include "Types.h"

namespace Onikiri 
{
    // 'PID' is Process ID.
    // It identifies a memory space in a emulator.
    static const int PID_INVALID = -1;

    // 'TID' is Hardware Thread ID.
    // It identifies hardware thread.
    static const int TID_INVALID = -1;


    class PhysicalResourceIF
    {
    public:
        virtual ~PhysicalResourceIF(){};

        virtual int  GetThreadCount() = 0;
        virtual void SetThreadCount(const int count) = 0;
        
        virtual int  GetTID(const int index) = 0;
        virtual void SetTID(const int index, const int tid) = 0;
    };
    

    class LogicalResourceIF
    {
    public:
        virtual ~LogicalResourceIF(){};

        virtual int GetPID() const = 0;
        virtual int GetTID() const = 0;
    };

    struct LogicalData
    {
        int pid;
        int tid;

        LogicalData( int newPId = PID_INVALID, int newTID = TID_INVALID ) : 
            pid( newPId ),
            tid( newTID )
        {
        }
    };

}; // namespace Onikiri

#endif // __RESOURCE_H__


