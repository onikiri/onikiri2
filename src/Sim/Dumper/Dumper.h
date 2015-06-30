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


#ifndef __DUMPER_H__
#define __DUMPER_H__

#include "Types.h"
#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/Dumper/DumpState.h"


namespace Onikiri 
{
    class Thread;
    class TraceDumper;
    class CountDumper;
    class VisualizationDumper;
    class Core;

    class Dumper : public ParamExchange
    {
        struct ThreadDumper
        {
            TraceDumper*         traceDumper;
            VisualizationDumper* visDumper;
            CountDumper*         countDumper;
        };

        typedef unordered_map<Thread*, ThreadDumper> DumperMap;
        typedef std::vector<ThreadDumper> DumperList;
        DumperMap m_dumperMap;
        DumperList m_dumperList;

        bool m_dumpEnabled;
        bool m_dumpEachThread;
        bool m_dumpEachCore;

        void DumpImpl(DUMP_STATE state, OpIterator op, int detail);
        void DumpStallBeginImpl(OpIterator op);
        void DumpStallEndImpl(OpIterator op);
        void SetCurrentCycleImpl( Thread* thread, s64 cycle );
        void SetCurrentInsnCountImpl( Thread* thread, s64 count );
        void DumpOpDependencyImpl( const OpIterator producerOp, const OpIterator consumerOp, DumpDependency type );
        void DumpRawStageImpl( OpIterator op, bool begin, const char*stage, DumpLane lane );
        void SetEnabledImpl( bool enabled );

        void CreateThreadDumper(
            ThreadDumper* threadDumper,
            const String& suffix, 
            PhysicalResourceArray<Core>& coreList 
        );
        void ReleaseDumper();
    public:
        BEGIN_PARAM_MAP("/Session/Environment/Dumper/")
            PARAM_ENTRY("@DumpEachThread",  m_dumpEachThread)
            PARAM_ENTRY("@DumpEachCore",    m_dumpEachCore)
        END_PARAM_MAP()

        Dumper();
        virtual ~Dumper();
        void Initialize( 
            PhysicalResourceArray<Core>& coreList,
            PhysicalResourceArray<Thread>& threadList 
        );
        void Finalize();

        bool IsEnabled()
        {
            return m_dumpEnabled;
        }

        void Dump(DUMP_STATE state, OpIterator op, int detail = -1)
        {
            if(!m_dumpEnabled)
                return;
            DumpImpl(state, op, detail);
        }

        void DumpStallBegin(OpIterator op)
        {
            if(!m_dumpEnabled)
                return;
            DumpStallBeginImpl(op);
        }

        void DumpStallEnd(OpIterator op)
        {
            if(!m_dumpEnabled)
                return;
            DumpStallEndImpl(op);
        }

        void SetCurrentCycle( Thread* thread, s64 cycle )
        {
            if(!m_dumpEnabled)
                return;
            SetCurrentCycleImpl( thread, cycle );
        }

        void SetCurrentInsnCount( Thread* thread, s64 count )
        {
            if(!m_dumpEnabled)
                return;
            SetCurrentInsnCountImpl( thread, count );
        }

        // Dump dependencies from producerOp to consumerOp.
        // See comments for DUMP_DEPENDENCY_TYPE.
        void DumpOpDependency(
            const OpIterator producerOp, 
            const OpIterator consumerOp, 
            DumpDependency type = DDT_WAKEUP 
        ){
            if(!m_dumpEnabled)
                return;
            DumpOpDependencyImpl( producerOp, consumerOp, type );
        }

        // Dump user defined stages.
        // See comments for VisualizationDumper::DumpRawStage and DumpLane.
        void DumpRawStage( OpIterator op, bool begin, const char*stage, DumpLane lane )
        {
            if(!m_dumpEnabled)
                return;
            DumpRawStageImpl( op, begin, stage, lane );
        }

        void SetEnabled( bool enabled )
        {
            SetEnabledImpl( enabled );
        }
    };

    extern Dumper g_dumper;

}; // namespace Onikiri

#endif // __DUMPER_H__

