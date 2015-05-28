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
// A cache system class.
//

#ifndef SIM_MEMORY_CACHE_CACHE_SYSTEM_H
#define SIM_MEMORY_CACHE_CACHE_SYSTEM_H

#include "Env/Param/ParamExchange.h"

#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Memory/Cache/CacheTypes.h"

namespace Onikiri
{
    class Cache;
    class Thread;

    class CacheSystem :
        public PhysicalResourceNode
    {
    public:
    
        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )

                PARAM_ENTRY( "@DataCacheHierarchy",     m_dataCacheHierarchyStr )
                PARAM_ENTRY( "@InsnCacheHierarchy",     m_insnCacheHierarchyStr )

                // Load instructions
                BEGIN_PARAM_PATH( "Load" )
                    PARAM_ENTRY( "@NumStoreForwarding",     m_numRetiredStoreForwarding )
                    PARAM_ENTRY( "@NumRetiredLoadAccess",   m_numRetiredLoadAccess )
                    PARAM_ENTRY( 
                        "@CacheAccessCoverage", 
                        m_loadAccessCoverage 
                    )
                    PARAM_ENTRY( 
                        "@CacheAccessDispersion", 
                        m_loadAccessDispersion 
                    )
                    PARAM_ENTRY( "@MissesPerKiloInstructions", m_loadMPKI );
                END_PARAM_PATH()
                
                // Store instructions
                BEGIN_PARAM_PATH( "Store" )
                    PARAM_ENTRY( "@NumRetiredStoreAccess",  m_numRetiredStoreAccess )
                    PARAM_ENTRY( 
                        "@CacheAccessCoverage", 
                        m_storeAccessCoverage 
                    )
                    PARAM_ENTRY( 
                        "@CacheAccessDispersion", 
                        m_storeAccessDispersion 
                    )
                    PARAM_ENTRY( "@MissesPerKiloInstructions", m_storeMPKI );
                END_PARAM_PATH()

                // Load + Store instructions
                BEGIN_PARAM_PATH( "Total" )
                    PARAM_ENTRY( "@NumRetiredMemoryAccess", m_numRetiredMemoryAccess )
                    PARAM_ENTRY( 
                        "@CacheAccessCoverage", 
                        m_memoryAccessCoverage 
                    )
                    PARAM_ENTRY( 
                        "@CacheAccessDispersion", 
                        m_memoryAccessDispersion 
                    )
                    PARAM_ENTRY( "@MissesPerKiloInstructions", m_totalMPKI );
                END_PARAM_PATH()
            END_PARAM_PATH()
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( Cache,  "firstLevelDataCache",  m_firstLvDataCache )
            RESOURCE_ENTRY( Cache,  "firstLevelInsnCache",  m_firstLvInsnCache )
            RESOURCE_ENTRY( Thread, "thread",   m_thread   )
        END_RESOURCE_MAP()

        CacheSystem();
        virtual ~CacheSystem();

        //
        // --- PhysicalResourceNode
        //
        
        virtual void Initialize( InitPhase phase );
        virtual void Finalize();

        // This method is called when a simulation mode is changed.
        virtual void ChangeSimulationMode( PhysicalResourceNode::SimulationMode mode );

        void Commit( OpIterator op );
        void Cancel( OpIterator op ){};
        void Retire( OpIterator op ){};
        void Flush( OpIterator op ){};

        Cache* GetFirstLevelDataCache() const { return m_firstLvDataCache; }
        Cache* GetFirstLevelInsnCache() const { return m_firstLvInsnCache; }

    protected:

        Cache*  m_firstLvDataCache;
        Cache*  m_firstLvInsnCache;
        PhysicalResourceArray<Thread> m_thread;
        SimulationMode m_mode;

        // Cache hierarchy information
        std::vector< Cache* > m_dataCacheHierarchy;
        std::vector< Cache* > m_insnCacheHierarchy;
        std::vector< std::string > m_dataCacheHierarchyStr;
        std::vector< std::string > m_insnCacheHierarchyStr;

        // Dispersion of where retired load instructions accessed to.
        // 0:L1, 1:L2, 2...
        std::vector< s64 >    m_loadAccessDispersion;
        std::vector< s64 >    m_storeAccessDispersion;
        std::vector< s64 >    m_memoryAccessDispersion;

        // The coverage of accesses for each cache hierarchy.
        // This is calculated from dispersions.
        std::vector< double > m_loadAccessCoverage;
        std::vector< double > m_storeAccessCoverage;
        std::vector< double > m_memoryAccessCoverage;

        // Miss per kilo instructions.
        std::vector< double > m_loadMPKI;
        std::vector< double > m_storeMPKI;
        std::vector< double > m_totalMPKI;

        s64 m_numRetiredStoreForwarding;
        s64 m_numRetiredLoadAccess;
        s64 m_numRetiredStoreAccess;
        s64 m_numRetiredMemoryAccess;

        // Get cache hierarchy information from a passed top level cache.
        void InitCacheHierarchy( 
            std::vector< Cache* >* hierarchy, 
            std::vector< std::string >* hierarchyStr,
            Cache* top 
        );
        
        // Get a cache level in the m_dataCacheHierarcy.
        int GetDataCacheLevel( Cache* cache );

        // Calculate coverage from the number of raw accesses.
        void CalculateCoverage(
            std::vector< double >* coverage, 
            const std::vector< s64 >& dispersion 
        );

        // Calculate MPKI from the number of raw accesses.
        void CalculateMissPerKiloInsns(
            std::vector< double >* mpki, 
            const std::vector< s64 >& dispersion,
            s64 retiredInsns
        );
    };

}; // namespace Onikiri

#endif // SIM_MEMORY_CACHE_CACHE_SYSTEM_H

