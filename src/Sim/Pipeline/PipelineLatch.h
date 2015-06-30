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


#ifndef SIM_PIPELINE_PIPELINE_LATCH_H
#define SIM_PIPELINE_PIPELINE_LATCH_H

#include "Utility/RuntimeError.h"
#include "Sim/Foundation/TimeWheel/ClockedResourceBase.h"
#include "Sim/Op/OpArray/OpArray.h"

namespace Onikiri 
{

    class PipelineLatch : public ClockedResourceBase
    {
    public:
        // Pipeline latch
        // PipelineNodeBase can belong to multiple cores (ex. Cache), 
        // thus OpList cannot be used. One OpList cannot be used by multiple cores.
        //typedef pool_list<OpIterator> List;
        enum MaxLatchSize { MAX_LATCH_SIZE = 256 };
        typedef fixed_sized_buffer< OpIterator, MAX_LATCH_SIZE, MaxLatchSize > List;
        typedef List::iterator iterator;

        iterator begin()    {   return m_latchOut.begin();      }
        iterator end()      {   return m_latchOut.end();        }
        iterator erase( iterator i )
                            {   return m_latchOut.erase( i );   }
        void pop_front()    {   m_latchOut.pop_front();         }
        size_t   size()     {   return m_latchOut.size();       }

    protected:
        typedef ClockedResourceBase BaseType;

        List m_latchIn;
        List m_latchOut;
        bool m_enableDumpStall;
        bool m_latchOutIsStalled;

        bool FindAndEraseFromLatch( List* latch, OpIterator op );
        void DumpStallBegin();
        void DumpStallEnd();

        // Update a latch
        void UpdateLatch();

    public:

        PipelineLatch( const char* name = "" );
        virtual void Receive( OpIterator op );
        void EnableDumpStall( bool enable );

        // A cycle end handler
        // Update the pipeline latch.
        virtual void End();

        // A cycle transition handler
        virtual void Transition();

        // Stall handlers, which are called in stall begin/end
        virtual void BeginStall();
        virtual void EndStall();
        virtual void Delete( OpIterator op );
    };

}; // namespace Onikiri

#endif // SIM_PIPELINE_PIPELINE_LATCH_H
