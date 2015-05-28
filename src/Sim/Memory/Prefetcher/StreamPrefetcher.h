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
// Stream prefetcher implementation.
// This implementation is based on a model described in the below paper.
// 
// "Feedback Directed Prefetching: Improving the Performance 
//  and Bandwidth-Efficiency of Hardware Prefetchers", 
// Santhosh Srinathyz et al., HPCA 07
//
//

#ifndef __STREAM_PREFETCHER_H__
#define __STREAM_PREFETCHER_H__

#include "Sim/Memory/Prefetcher/PrefetcherBase.h"
#include "Sim/Memory/Cache/CacheExtraStateTable.h"

namespace Onikiri
{

    class StreamPrefetcher : public PrefetcherBase
    {
    protected:

        int m_distance;
        int m_degree;
        int m_streamTableSize;
        int m_trainingWindowSize;
        int m_trainingThreshold;

        u64 m_effectiveDistance;        // An effective distance is calculated 
                                        // by a 'distance' parameter and a line size.
        u64 m_effectiveTrainingWindow;  //

        // Statistics
        s64 m_numAllocatedStream;
        s64 m_numMonitioredStream;
        s64 m_numAscendingMonitioredStream;
        s64 m_numDesscendingMonitioredStream;
        double m_numAvgMonitoredStreamLength;
        s64 m_numTrainingAccess;
        s64 m_numAlocatingAccess;

        enum StreamStatus
        {
            SS_INVALID,
            SS_TRAINING,    // Including 'Allocated' state as 'trainingCount' == 0.
            SS_MONITOR
        };

        struct Stream
        {
            StreamStatus status;    // Stream status

            Addr orig;      // An first accessed address of a stream 
            Addr addr;      // A current 'start' address of a stream 

            int  count;     // Access count 
            bool ascending; // Stream direction

            Stream()
            {
                Reset();
            }

            void Reset()
            {
                status = SS_INVALID;
                ascending = true;
                count = 0;
            }

            String ToString() const;
        };

        typedef 
            shttl::table< Stream > 
            StreamTable;

        StreamTable m_streamTable;

        // Do prefetch an address specified by a stream and update a stream. 
        void Prefetch( const CacheAccess& kickerAccess, Stream* stream );

        // Returns whether 'addr' is in a window specified by the remaining arguments or not .
        // 'ascending' means a direction of a window. 
        bool IsInWindow( const Addr& addr, const Addr& start, u64 windowSize, bool ascending );
        bool IsInWindow( u64 addr, u64 start, u64 windowSize, bool ascending );

        // Check and update entries with SS_MONITOR status in the stream table.
        // Returns whether any entries are updated or not.
        bool UpdateMonitorStream( const CacheAccess& access );

        // Check and update entries with SS_TRAINING status in the stream table.
        bool UpdateTrainingStream( const CacheAccess& access );

        // Allocate a new entry in the stream table.
        void AllocateStream( const CacheAccess& access );

    public:
        
        StreamPrefetcher();
        ~StreamPrefetcher();

        // --- PrefetcherIF
        virtual void OnCacheAccess( Cache* cache, const CacheAccess& access, bool hit );

        // --- PhysicalResourceNode
        virtual void Initialize(InitPhase phase);

        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY( "@Distance",        m_distance );
                PARAM_ENTRY( "@Degree",          m_degree );
                PARAM_ENTRY( "@StreamTableSize", m_streamTableSize );
                PARAM_ENTRY( "@TrainingWindowSize", m_trainingWindowSize );
                PARAM_ENTRY( "@TrainingThreashold", m_trainingThreshold );
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
                PARAM_ENTRY( "@NumAllocatedStream", m_numAllocatedStream );
                PARAM_ENTRY( "@NumMonitioredStream", m_numMonitioredStream );
                PARAM_ENTRY( "@NumAscendingMonitioredStream",   m_numAscendingMonitioredStream );
                PARAM_ENTRY( "@NumDesscendingMonitioredStream", m_numDesscendingMonitioredStream );
                PARAM_ENTRY( "@NumAvgMonitoredStreamLength",    m_numAvgMonitoredStreamLength );
                PARAM_ENTRY( "@NumTrainingAccess",  m_numTrainingAccess );
                PARAM_ENTRY( "@NumAlocatingAccess", m_numAlocatingAccess );
            END_PARAM_PATH()
            CHAIN_BASE_PARAM_MAP( PrefetcherBase )
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            CHAIN_BASE_RESOURCE_MAP( PrefetcherBase )
        END_RESOURCE_MAP()
    };


}; // namespace Onikiri

#endif // __STREAM_PREFETCHER_H__

