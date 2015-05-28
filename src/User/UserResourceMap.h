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
// Additional type map for user defined classes.
// This header file is included from "Sim/Resource/Builder/ResourceFactory.cpp"
//
//  Ex. :
//    BEGIN_USER_RESOURCE_TYPE_MAP()
//        RESOURCE_INTERFACE_ENTRY(Core)
//        RESOURCE_TYPE_ENTRY(Core)
//    END_USER_RESOURCE_TYPE_MAP()
//

#include "Samples/SampleNullModule.h"
#include "Samples/SampleHookModule.h"
#include "Samples/SampleBPred.h"
#include "Samples/SamplePrefetcher.h"

BEGIN_USER_RESOURCE_TYPE_MAP()

    // You can remove the following if you don't need the samples.

#ifdef USE_SAMPLE_NULL
    RESOURCE_TYPE_ENTRY( SampleNull )
#endif

#ifdef USE_SAMPLE_HOOK_MODULE
    RESOURCE_TYPE_ENTRY( SampleHookModule )
#endif

#ifdef USE_SAMPLE_BPRED
    RESOURCE_TYPE_ENTRY( SampleAlwaysHitBrDirPredictor )
#endif

#ifdef USE_SAMPLE_PREFETCHER
    RESOURCE_TYPE_ENTRY( SamplePrefetcher )
#endif

END_USER_RESOURCE_TYPE_MAP()

