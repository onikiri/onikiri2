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


#ifndef __PHYREG_H__
#define __PHYREG_H__

#include "Sim/Dependency/Dependency.h"

namespace Onikiri 
{

    //
    // 物理レジスタのクラス
    // 

    class PhyReg : public Dependency 
    {
    public:
        PhyReg( int numScheduler, int phyRegNo ) :
            Dependency(numScheduler),
            m_val(0),
            m_phyRegNo(phyRegNo)
        {
        }

        ~PhyReg()
        {
        }

        const u64 GetVal() const
        {
            return m_val;
        }

        void SetVal(const u64& val)
        {
            m_val = val;
        }

        const int GetPhyRegNo() const
        {
            return m_phyRegNo;
        }

    private:
        
        // 値
        u64 m_val;

        // 物理レジスタ番号
        int m_phyRegNo;
        
    };

};

#endif // __PHYREG_H__

