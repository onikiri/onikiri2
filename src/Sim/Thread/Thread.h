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


#ifndef SIM_THREAD_THREAD_H
#define SIM_THREAD_THREAD_H

#include "Sim/Foundation/Hook/HookDecl.h"
#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/Foundation/Checkpoint/CheckpointedData.h"


namespace Onikiri 
{
    class EmulatorIF;
    class Core;
    class InorderList;
    class CheckpointMaster;
    class MemOrderManager;
    class RegDepPredIF;
    class MemDepPredIF;
    class Recoverer;

    class Thread : 
        public PhysicalResourceNode
    {
        
    public:
        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( EmulatorIF,         "emulator",         m_emulator )
            RESOURCE_ENTRY( InorderList,        "inorderList",      m_inorderList )
            RESOURCE_ENTRY( MemOrderManager,    "memOrderManager",  m_memOrderManager )
            RESOURCE_ENTRY( CheckpointMaster,   "checkpointMaster", m_checkpointMaster )
            RESOURCE_ENTRY( RegDepPredIF,       "regDepPred",       m_regDepPred )
            RESOURCE_ENTRY( MemDepPredIF,       "memDepPred",       m_memDepPred )
            RESOURCE_ENTRY( Core,               "core",             m_core )
            RESOURCE_ENTRY( Recoverer,          "recoverer",        m_recoverer )
        END_RESOURCE_MAP()

        Thread();
        virtual ~Thread();
        void Initialize(InitPhase phase);

        int GetTID();
        int GetTID( const int index );
        void SetLocalThreadID(int localTID);
        int  GetLocalThreadID() const;

        bool IsActive();
        void Activate( bool active );
        void InitializeContext(PC pc);

        void SetFetchPC(const PC& pc);
        PC   GetFetchPC() const;
        u64  GetOpRetiredID();
        u64  GetOpSerialID();
        void AddOpRetiredID(u64 num);
        void AddOpSerialID(u64 num);

        void SetThreadCount(const int count);
        // accessors
        Core*               GetCore()               const   { return m_core;                }
        InorderList*        GetInorderList()        const   { return m_inorderList;         }
        MemOrderManager*    GetMemOrderManager()    const   { return m_memOrderManager;     }
        CheckpointMaster*   GetCheckpointMaster()   const   { return m_checkpointMaster;    }
        RegDepPredIF*       GetRegDepPred()         const   { return m_regDepPred;          }
        MemDepPredIF*       GetMemDepPred()         const   { return m_memDepPred;          }
        Recoverer*          GetRecoverer()          const   { return m_recoverer;           }

    private:
        // member variables
        EmulatorIF*         m_emulator;
        Core*               m_core;
        InorderList*        m_inorderList;
        CheckpointMaster*   m_checkpointMaster;
        MemOrderManager*    m_memOrderManager;
        RegDepPredIF*       m_regDepPred;
        MemDepPredIF*       m_memDepPred;
        Recoverer*          m_recoverer;

        int m_localTID;
        bool m_active;
        CheckpointedData<PC> m_fetchPC;

        // リタイアした順番に振られるOpのインデックス
        // Fetchの段階で振るため、チェックポイントで管理する
        CheckpointedData<u64> m_retiredOpID; 
        u64 m_serialOpID; 

    };  // class Thread

}   // namespace Onikiri 

#endif  // #ifndef SIM_THREAD_THREAD_H


