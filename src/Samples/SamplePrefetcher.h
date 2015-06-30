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
// This is a sample class to demonstrate how to implement 
// a prefetcher module.
// You can activate this class by defining USE_SAMPLE_PREFETCHER
// in this file.
//

#ifndef SAMPLES_SAMPLE_PREFETCHER_H
#define SAMPLES_SAMPLE_PREFETCHER_H

// USE_SAMPLE_PREFETCHER is referred in this file, 'User/UserResourceMap.h'
// and  'User/UserInit.h'.
//#define USE_SAMPLE_PREFETCHER 1


#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/Memory/Prefetcher/PrefetcherBase.h"

// All Onikiri classes are placed in a 'Onikiri' namespace.
namespace Onikiri
{
    class Core;

    // PrefetcherBase inherits PhysicalResourceNode so
    // SamplePrefetcher does not need to inherit PhysicalResourceNode.
    class SamplePrefetcher : public PrefetcherBase
    {
    public:

        SamplePrefetcher()
        {
        }
        virtual ~SamplePrefetcher()
        {
        }

        //
        // --- PrefetcherBase
        // Initialize and Finalize must be implemented.
        //
        virtual void Initialize( InitPhase phase )
        {
            // Initialize of a base class must be called.
            PrefetcherBase::Initialize( phase );

            if( phase == INIT_PRE_CONNECTION ){

                // After constructing and before object connection.
                // LoadParam() must be called in this phase or later.
                LoadParam();

            }
            else if ( phase == INIT_POST_CONNECTION ){
            }
        }

        virtual void Finalize()
        {
            ReleaseParam();

            // Finalize of a base class must be called.
            PrefetcherBase::Finalize();
        }

        virtual void OnCacheAccess( Cache* cache, const CacheAccess& access, bool hit )
        {
            // If the cache access is miss, prefetch a next line.
            if( !hit ){
                CacheAccess prefetch = access;

                // If you need an original address, it can be obtained from 'op'.
                // 'access' may be masked by a line offset.
                //CacheAccess access( op->GetMemAccess(), op, CacheAccess::OT_READ );
                
                prefetch.type = CacheAccess::OT_PREFETCH;
                prefetch.address.address += PrefetcherBase::m_lineSize;
                
                // Prefetch target is set to PrefetcherBase from XML.
                Prefetch( prefetch );
            }
        }

        // Parameter & resource map.
        // These contents in this map are bound to XML in SampleDefaultParam.h.
        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
            END_PARAM_PATH()
            
            // Chain to a map of a base class.
            CHAIN_BASE_PARAM_MAP( PrefetcherBase )
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            // Chain to a map of a base class.
            CHAIN_BASE_RESOURCE_MAP( PrefetcherBase )
        END_RESOURCE_MAP()

    };

    //
    // Default parameters and connections for SamplePrefetcher.
    // This XML overwrite XML in DefaultParam.xml.
    //

#ifdef USE_SAMPLE_PREFETCHER

    static char g_samplePrefetcherParam[] = "       \n\
                                                    \n\
    <?xml version='1.0' encoding='UTF-8'?>          \n\
    <Session>                                       \n\
                                                    \n\
      <Simulator>                                   \n\
        <Configurations>                            \n\
          <DefaultConfiguration>                    \n\
                                                    \n\
            <Structure>                             \n\
              <Copy>                                \n\
                                                    \n\
                <SamplePrefetcher                   \n\
                  Count = 'CoreCount'               \n\
                  Name  = 'samplePrefetcher'        \n\
                >                                   \n\
                    <Connection                     \n\
                        Name = 'cacheL2'            \n\
                        To = 'target'               \n\
                    />                              \n\
                </SamplePrefetcher>                 \n\
                                                    \n\
                <Cache Name = 'cacheL1D' />         \n\
                <Cache Name = 'cacheL1I' />         \n\
                <Cache Name = 'cacheL2' >           \n\
                    <Connection                     \n\
                        Name = 'samplePrefetcher'   \n\
                        To = 'prefetcher'           \n\
                    />                              \n\
                </Cache>                            \n\
                                                    \n\
                                                    \n\
              </Copy>                               \n\
            </Structure>                            \n\
                                                    \n\
            <Parameter>                             \n\
                                                    \n\
              <SamplePrefetcher                     \n\
                EnablePrefetch = '1'                \n\
                OffsetBitSize = '6'                 \n\
                Name  = 'samplePrefetcher'          \n\
              >                                     \n\
              </SamplePrefetcher>                   \n\
                                                    \n\
            </Parameter>                            \n\
          </DefaultConfiguration>                   \n\
        </Configurations>                           \n\
      </Simulator>                                  \n\
                                                    \n\
    </Session>                                      \n\
                                                    \n\
                                                    \n\
    ";

#endif  // #ifdef USE_SAMPLE_PREFETCHER


}


#endif
