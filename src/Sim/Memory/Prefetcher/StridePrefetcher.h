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
// Stride prefetcher implementation.
//

#ifndef SIM_MEMORY_PREFETCHER_STRIDE_PREFETCHER_H
#define SIM_MEMORY_PREFETCHER_STRIDE_PREFETCHER_H

#include "Sim/Memory/Prefetcher/PrefetcherBase.h"
#include "Sim/Memory/Cache/CacheExtraStateTable.h"

namespace Onikiri
{
    
    class StridePrefetcher : public PrefetcherBase
    {
        int m_distance;
        int m_degree;
        int m_streamTableSize;

        // Statistics
        s64 m_numPrefetchingStream;
        double m_numAvgPrefetchStreamLength;

        enum StreamStatus
        {
            SS_INVALID,
            SS_ALLOCATED,
            SS_PREFETCHING
        };

        // A confidence counter has hysteresis characteristics.
        static const int CONFIDENCE_INIT = 0;
        static const int CONFIDENCE_MIN  = 0;
        static const int CONFIDENCE_MAX  = 7;
        static const int CONFIDENCE_INC  = 1;
        static const int CONFIDENCE_DEC  = 4;
        static const int CONFIDENCE_PREDICTION_THREASHOLD = 7;

        // stride access stream
        struct Stream
        {
            StreamStatus status;    // Stream status

            Addr pc;        // An address of an instruction that begins a stream.

            Addr orig;      // An first accessed address of a stride 
            Addr addr;      // A current address of a stream 
            s64  stride;    // A stride 

            int  count;                 // Access count of this stream.
            int  streamLength;          // The length of stride access stream.
            shttl::counter<> confidence;    // Prediction confidence.

            Stream()
            {
                Reset();
                confidence.set(
                    CONFIDENCE_INIT,
                    CONFIDENCE_MIN,
                    CONFIDENCE_MAX,
                    CONFIDENCE_INC,
                    CONFIDENCE_DEC,
                    CONFIDENCE_PREDICTION_THREASHOLD
                );
            }

            void Reset()
            {
                status = SS_INVALID;
                stride = 0;

                count = 0;
                streamLength = 0;
                confidence.reset();
            }

            String ToString() const;
        };

        typedef 
            shttl::table< Stream > 
            StreamTable;

        StreamTable m_streamTable;

        // Do prefetch an address specified by a stream and update a stream. 
        void Prefetch( OpIterator op, Stream* stream );

    public:
        
        StridePrefetcher();
        ~StridePrefetcher();

        // --- PrefetcherIF
        virtual void OnCacheAccess( Cache* cache, const CacheAccess& access, bool hit );

        // --- PhysicalResourceNode
        virtual void Initialize(InitPhase phase);

        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY( "@Distance",        m_distance );
                PARAM_ENTRY( "@Degree",          m_degree );
                PARAM_ENTRY( "@StreamTableSize", m_streamTableSize );
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
                PARAM_ENTRY( "@NumPrefetchingStream",   m_numPrefetchingStream );
                PARAM_ENTRY( "@NumAvgPrefetchStreamLength", m_numAvgPrefetchStreamLength );
            END_PARAM_PATH()
            CHAIN_BASE_PARAM_MAP( PrefetcherBase )
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            CHAIN_BASE_RESOURCE_MAP( PrefetcherBase )
        END_RESOURCE_MAP()
    };


}; // namespace Onikiri

#endif // SIM_MEMORY_PREFETCHER_STRIDE_PREFETCHER_H

