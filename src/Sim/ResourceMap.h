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
// This file defines the mapping of resource names used in
// XML files and resource types defined in cpp files.
//
// This file is included from Resource/Builder/ResourceFactory.cpp
//

#include "Sim/Foundation/Checkpoint/CheckpointMaster.h"
#include "Sim/System/GlobalClock.h"
#include "Sim/System/ForwardEmulator.h"

#include "Sim/Core/Core.h"
#include "Sim/Thread/Thread.h"

#include "Sim/InorderList/InorderList.h"
#include "Sim/Memory/MemOrderManager/MemOrderManager.h"

#include "Sim/Register/RegisterFile.h"
#include "Sim/Register/RegisterFreeList.h"
#include "Sim/Memory/Cache/Cache.h"
#include "Sim/Memory/Cache/CacheSystem.h"
#include "Sim/Memory/Prefetcher/PrefetcherBase.h"
#include "Sim/Memory/Prefetcher/StreamPrefetcher.h"
#include "Sim/Memory/Prefetcher/StridePrefetcher.h"

#include "Sim/ExecUnit/ExecUnit.h"
#include "Sim/ExecUnit/PipelinedExecUnit.h"
#include "Sim/ExecUnit/MemExecUnit.h"
#include "Sim/ExecUnit/ExecLatencyInfo.h"

#include "Sim/Pipeline/Fetcher/Fetcher.h"
#include "Sim/Pipeline/Fetcher/Steerer/RoundRobinFetchThreadSteerer.h"
#include "Sim/Pipeline/Fetcher/Steerer/IcountFetchThreadSteerer.h"
#include "Sim/Pipeline/Dispatcher/Dispatcher.h"
#include "Sim/Pipeline/Renamer/Renamer.h"
#include "Sim/Pipeline/Dispatcher/Steerer/OpCodeSteerer.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/Pipeline/Retirer/Retirer.h"

#include "Sim/Pipeline/Scheduler/IssueSelector/AgeIssueSelector.h"
#include "Sim/Pipeline/Scheduler/IssueSelector/InorderIssueSelector.h"
#include "Sim/Pipeline/Scheduler/IssueSelector/IssueSelector.h"


#include "Sim/Predictor/BPred/BPred.h"
#include "Sim/Predictor/BPred/BTB.h"
#include "Sim/Predictor/BPred/GlobalHistory.h"
#include "Sim/Predictor/BPred/GShare.h"
#include "Sim/Predictor/BPred/PHT.h"
#include "Sim/Predictor/BPred/RAS.h"

#include "Sim/Predictor/DepPred/MemDepPred/ConservativeMemDepPred.h"
#include "Sim/Predictor/DepPred/MemDepPred/OptimisticMemDepPred.h"
#include "Sim/Predictor/DepPred/MemDepPred/StoreSet.h"
#include "Sim/Predictor/DepPred/MemDepPred/MemDepPred.h"
#include "Sim/Predictor/DepPred/MemDepPred/PerfectMemDepPred.h"
#include "Sim/Predictor/DepPred/RegDepPred/RMT.h"

#include "Sim/Predictor/LatPred/LatPredResult.h"
#include "Sim/Predictor/LatPred/LatPred.h"
#include "Sim/Predictor/HitMissPred/CounterBasedHitMissPred.h"
#include "Sim/Predictor/HitMissPred/StaticHitMissPred.h"

#include "Sim/Recoverer/Recoverer.h"


//
// --- Type map
// 
// RESOURCE_TYPE_ENTRY : 
//   Register a type.
//
// RESOURCE_INTERFACE_ENTRY : 
//   Register a interface.
//
// Ex.
// RESOURCE_TYPE_ENTRY( Module ) 
// This statement maps a 
// 'Module' node in XML files and 
// 'class Module' in cpp source files.
//
//

BEGIN_RESOURCE_TYPE_MAP()

    RESOURCE_INTERFACE_ENTRY(EmulatorIF);
    RESOURCE_TYPE_ENTRY(CheckpointMaster)
    RESOURCE_TYPE_ENTRY(GlobalClock)
    RESOURCE_TYPE_ENTRY(ForwardEmulator)

    RESOURCE_TYPE_ENTRY(Core)
    RESOURCE_TYPE_ENTRY(Thread)
    RESOURCE_TYPE_ENTRY(InorderList)

    RESOURCE_TYPE_ENTRY(RMT)
    RESOURCE_INTERFACE_ENTRY(RegDepPredIF)
    RESOURCE_TYPE_ENTRY(RegisterFile)
    RESOURCE_TYPE_ENTRY(RegisterFreeList)

    RESOURCE_TYPE_ENTRY(MemOrderManager)
    RESOURCE_TYPE_ENTRY(Cache)
    RESOURCE_TYPE_ENTRY(CacheSystem)

    RESOURCE_INTERFACE_ENTRY(PrefetcherIF)
    RESOURCE_TYPE_ENTRY(StreamPrefetcher)
    RESOURCE_TYPE_ENTRY(StridePrefetcher)

    RESOURCE_TYPE_ENTRY(ExecUnit)
    RESOURCE_TYPE_ENTRY(PipelinedExecUnit)
    RESOURCE_TYPE_ENTRY(MemExecUnit)
    RESOURCE_INTERFACE_ENTRY(ExecUnitIF)
    RESOURCE_TYPE_ENTRY(ExecLatencyInfo)

    RESOURCE_INTERFACE_ENTRY(PipelineNodeIF)
    RESOURCE_TYPE_ENTRY(Fetcher)
    RESOURCE_TYPE_ENTRY(Renamer)
    RESOURCE_TYPE_ENTRY(Dispatcher)
    RESOURCE_TYPE_ENTRY(Scheduler)
    RESOURCE_TYPE_ENTRY(Retirer)

    RESOURCE_INTERFACE_ENTRY(FetchThreadSteererIF)
    RESOURCE_TYPE_ENTRY(RoundRobinFetchThreadSteerer)
    RESOURCE_TYPE_ENTRY(IcountFetchThreadSteerer)

    RESOURCE_INTERFACE_ENTRY(DispatchSteererIF)
    RESOURCE_TYPE_ENTRY(OpCodeDispatchSteerer)

    RESOURCE_INTERFACE_ENTRY(IssueSelectorIF)
    RESOURCE_TYPE_ENTRY(AgeIssueSelector)
    RESOURCE_TYPE_ENTRY(InorderIssueSelector)
    RESOURCE_TYPE_ENTRY(IssueSelector)

    RESOURCE_TYPE_ENTRY(BPred)
    RESOURCE_TYPE_ENTRY(BTB)
    RESOURCE_TYPE_ENTRY(GlobalHistory)
    RESOURCE_INTERFACE_ENTRY(DirPredIF)
    RESOURCE_TYPE_ENTRY(GShare)
    RESOURCE_TYPE_ENTRY(PHT)
    RESOURCE_TYPE_ENTRY(RAS)

    RESOURCE_TYPE_ENTRY(ConservativeMemDepPred)
    RESOURCE_TYPE_ENTRY(OptimisticMemDepPred)
    RESOURCE_TYPE_ENTRY(StoreSet)
    RESOURCE_TYPE_ENTRY(MemDepPred)
    RESOURCE_TYPE_ENTRY(PerfectMemDepPred)
    RESOURCE_INTERFACE_ENTRY(MemDepPredIF)

    RESOURCE_TYPE_ENTRY(LatPred)
    RESOURCE_TYPE_ENTRY(CounterBasedHitMissPred)
    RESOURCE_TYPE_ENTRY(OptimisticHitMissPred)
    RESOURCE_TYPE_ENTRY(PessimisticHitMissPred)
    RESOURCE_INTERFACE_ENTRY(HitMissPredIF)

    RESOURCE_TYPE_ENTRY(Recoverer)

END_RESOURCE_TYPE_MAP()

