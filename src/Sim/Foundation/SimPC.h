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
// PC Utility class
//

#include "Interface/Addr.h"
#include "Sim/ISAInfo.h"

#ifndef SIM_PC_H
#define SIM_PC_H


namespace Onikiri
{

    class SimPC : public PC
    {
    public:

        SimPC()
        {
        }

        SimPC(int pid, int tid, u64 address) :
            PC(pid, tid, address)
        {
        }

        SimPC(const PC& pc) :
            PC(pc)
        {
        }

        void operator =(const PC& pc)
        {
            pid     = pc.pid;
            tid     = pc.tid;
            address = pc.address;
        }

        bool operator ==(const PC& pc) const
        {
            return pc.operator ==(*this);
        }

        bool operator !=(const PC& pc) const
        {
            return pc.operator !=(*this);
        }
    };

}   // namespace Onikiri

#endif
