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


#ifndef SIM_USER_USER_INIT_H
#define SIM_USER_USER_INIT_H

#include "Utility/String.h"
#include "User/UserDefaultParam.h"

// You can remove the following if you don't need the samples.
#include "Samples/SampleNullModule.h"
#include "Samples/SampleHookModule.h"
#include "Samples/SampleBPred.h"
#include "Samples/SamplePrefetcher.h"

namespace Onikiri
{
    static void InitializeUserDefaultParameter(
        std::vector<String>* defaultParams 
    ){
        defaultParams->push_back( g_userDefaultParam );


        // You can remove the following if you don't need the samples.
        
        
#ifdef USE_SAMPLE_NULL  // for 'SampleAlwaysHitBrDirPredictor'
        defaultParams->push_back( g_sampleNullParam );
#endif
        
#ifdef USE_SAMPLE_HOOK_MODULE   // for 'SampleHookModule'
        defaultParams->push_back( g_sampleHookModuleParam );
#endif

#ifdef USE_SAMPLE_BPRED // for 'SampleAlwaysHitBrDirPredictor'
        defaultParams->push_back( g_sampleBPredDefaultParam );
#endif

#ifdef USE_SAMPLE_PREFETCHER        // for 'SamplePrefetcher'
        defaultParams->push_back( g_samplePrefetcherParam );
#endif
    }
}

#endif



