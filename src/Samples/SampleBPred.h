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
// a branch predictor module.
// You can activate this class by defining USE_SAMPLE_BPRED in this file.
//

#ifndef SAMPLES_SAMPLE_BPRED_H
#define SAMPLES_SAMPLE_BPRED_H

// USE_SAMPLE_BPRED is referred in this file, 'User/UserResourceMap.h' 
// and  'User/UserInit.h'.
//#define USE_SAMPLE_BPRED 1


#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/Predictor/BPred/DirPredIF.h"

// All Onikiri classes are placed in a 'Onikiri' namespace.
namespace Onikiri
{
    class Core;

    class SampleAlwaysHitBrDirPredictor : 
        public PhysicalResourceNode,
        public DirPredIF
    {
    public:

        SampleAlwaysHitBrDirPredictor() : 
            m_enableCount( false ),
            m_numPredicted( 0 ),
            m_core( 0 )
        {
        }

        virtual ~SampleAlwaysHitBrDirPredictor()
        {
        }

        //
        // --- PhysicalResourceNode
        // Initialize and Finalize must be implemented.
        //
        virtual void Initialize( InitPhase phase )
        {
            if( phase == INIT_PRE_CONNECTION ){

                // After constructing and before object connection.
                // LoadParam() must be called in this phase or later.
                LoadParam();

            }
            else if ( phase == INIT_POST_CONNECTION ){

                // After connection
                if( m_core == NULL ){
                    THROW_RUNTIME_ERROR( "'m_core' is not connected correctly." );
                }

            }
        }
        virtual void Finalize()
        {
            ReleaseParam();
        }

        // Parameter & resource map.
        // These contents in this map are bound to XML in SampleDefaultParam.h.
        BEGIN_PARAM_MAP("")
            BEGIN_PARAM_PATH( GetParamPath() )
                PARAM_ENTRY( "@EnableCount", m_enableCount )
            END_PARAM_PATH()
            BEGIN_PARAM_PATH( GetResultPath() )
                PARAM_ENTRY( "@NumPredicted", m_numPredicted )
            END_PARAM_PATH()
        END_PARAM_MAP()

        BEGIN_RESOURCE_MAP()
            RESOURCE_ENTRY( Core, "core", m_core )
        END_RESOURCE_MAP()

        //
        // --- DirPredIF
        //
        virtual bool Predict(  OpIterator op, PC pc )
        {
            if( m_enableCount ){
                m_numPredicted++;
            }

            return true;    // Always predicted to hit.
        }
        virtual void Finished( OpIterator op ){}
        virtual void Retired(  OpIterator op ){}



    private:
        bool  m_enableCount;
        s64   m_numPredicted;
        Core* m_core;
    };


#ifdef USE_SAMPLE_BPRED

    //
    // Default parameters and connections for SampleAlwaysHitBrDirPredictor.
    // This XML overwrite XML in DefaultParam.xml.
    //

    static const char g_sampleBPredDefaultParam[] = "   \n\
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
                <SampleAlwaysHitBrDirPredictor      \n\
                  Count = 'CoreCount'               \n\
                  Name  = 'sampleBPred'             \n\
                >                                   \n\
                  <Core Name = 'core' />            \n\
                </SampleAlwaysHitBrDirPredictor>    \n\
                                                    \n\
                <BPred Name = 'bPred'>              \n\
                  <Connection Name='sampleBPred' To='dirPred'/> \n\
                </BPred>                            \n\
                                                    \n\
              </Copy>                               \n\
            </Structure>                            \n\
                                                    \n\
            <Parameter>                             \n\
                                                    \n\
              <SampleAlwaysHitBrDirPredictor        \n\
                EnableCount = '1'                   \n\
                Name  = 'sampleBPred'               \n\
              >                                     \n\
              </SampleAlwaysHitBrDirPredictor>      \n\
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
#endif // #ifdef USE_SAMPLE_BPRED

}   // namespace Onikiri


#endif// #ifndef SAMPLES_SAMPLE_BPRED_H
