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


#ifndef __EMULATOR_WRAPPER_H
#define __EMULATOR_WRAPPER_H

#include "Interface/EmulatorIF.h"
#include "Sim/Foundation/Resource/ResourceNode.h"

namespace Onikiri 
{
    class EmulatorWrapper : 
        public EmulatorIF, 
        public PhysicalResourceNode
    {
        EmulatorIF* m_body;
    public:

        EmulatorWrapper()
        {
            m_body = NULL;
        }

        ~EmulatorWrapper()
        {
            ReleaseParam();
        }

        void SetEmulator( EmulatorIF* body )
        {
            m_body = body;
        }

        //
        // PhysicalResourceNode
        //
        BEGIN_RESOURCE_MAP()
        END_RESOURCE_MAP()

        void Initialize(InitPhase phase)
        {
        };


        //
        // EmulatorIF
        //
        std::pair<OpInfo**, int> GetOp(PC pc)
        {
            return m_body->GetOp( pc );
        }

        MemIF* GetMemImage()
        {
            return m_body->GetMemImage();
        }

        void Execute( OpStateIF* opStateIF, OpInfo* opInfo )
        {
            m_body->Execute( opStateIF, opInfo );
        }

        void Commit( OpStateIF* opStateIF, OpInfo* opInfo )
        {
            m_body->Commit( opStateIF, opInfo );
        }

        int GetProcessCount() const
        {
            return m_body->GetProcessCount();
        }

        PC GetEntryPoint(int pid) const
        {
            return m_body->GetEntryPoint( pid );
        }

        u64 GetInitialRegValue(int pid, int index) const
        {
            return m_body->GetInitialRegValue( pid, index );
        }

        ISAInfoIF* GetISAInfo()
        {
            return m_body->GetISAInfo();
        }

        PC Skip(PC pc, u64 skipCount, u64* regArray, u64* executedInsnCount, u64* executedOpCount)
        {
            return m_body->Skip( pc, skipCount, regArray, executedInsnCount, executedOpCount );
        }

        void SetExtraOpDecoder( ExtraOpDecoderIF* extraOpDecoder ) 
        {
            m_body->SetExtraOpDecoder( extraOpDecoder );
        }
    };
}

#endif // #ifndef __EMULATOR_WRAPPER_H
