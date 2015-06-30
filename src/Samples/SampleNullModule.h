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
// This is a sample null class to demonstrate how to implement a 
// minimum module. You can activate this class by defining USE_SAMPLE_NULL.
//

#ifndef SAMPLES_SAMPLE_NULL_H
#define SAMPLES_SAMPLE_NULL_H

// USE_SAMPLE_NULL is referred in 'User/UserResourceMap.h' and 'User/UserInit.h'.
//#define USE_SAMPLE_NULL 1



#include "Sim/Foundation/Resource/ResourceNode.h"

// All Onikiri classes are placed in a 'Onikiri' namespace.
namespace Onikiri
{
    // All modules must inherit PhysicalResourceNode.
    class SampleNull : public PhysicalResourceNode
    {
    public:

        SampleNull(){};
        virtual ~SampleNull(){};

        // Initialize and Finalize must be implemented.
        virtual void Initialize( InitPhase phase )
        {
            if( phase == INIT_PRE_CONNECTION ){
                // After constructing and before object connection.
                // LoadParam() must be called in this phase or later.
                LoadParam();
            }
            else if ( phase == INIT_POST_CONNECTION ){
                // After connection
            }
            g_env.Print( "SampleNull::Initialize is called.\n" );
        }
        virtual void Finalize()
        {
            // ReleaseParam must be called in Finalize.
            ReleaseParam();
            g_env.Print( "SampleNull::Finalize() is called.\n" );
        };

        // Parameter & resource map.
        // These contents in this map are bound to the below XML.
        // Binding is defined in 'User/UserResourceMap.h' and 'User/UserInit.h'.
        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
            END_PARAM_PATH()
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
        END_RESOURCE_MAP()
    };

#ifdef USE_SAMPLE_NULL
    // Default parameters and connections for SampleNull.
    // This XML overwrite XML in DefaultParam.xml partially.
    static const char g_sampleNullParam[] = "       \n\
                                                    \n\
    <?xml version='1.0' encoding='UTF-8'?>          \n\
    <Session>                                       \n\
      <Simulator>                                   \n\
        <Configurations>                            \n\
          <DefaultConfiguration>                    \n\
                                                    \n\
            <Structure>                             \n\
              <Copy>                                \n\
                <SampleNull                         \n\
                  Count = 'CoreCount'               \n\
                  Name  = 'sampleNull'              \n\
                />                                  \n\
              </Copy>                               \n\
            </Structure>                            \n\
                                                    \n\
            <Parameter>                             \n\
              <SampleNull                           \n\
                Name  = 'sampleNull'                \n\
              />                                    \n\
            </Parameter>                            \n\
          </DefaultConfiguration>                   \n\
        </Configurations>                           \n\
      </Simulator>                                  \n\
    </Session>                                      \n\
    ";

#endif  // #ifdef USE_SAMPLE_NULL

}

#endif  // #ifndef SAMPLES_SAMPLE_NULL_H
