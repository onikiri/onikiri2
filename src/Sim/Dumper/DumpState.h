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


#ifndef SIM_DUMPER_DUMP_STATE_H
#define SIM_DUMPER_DUMP_STATE_H


namespace Onikiri 
{
    enum DUMP_STATE
    {
        DS_FETCH = 0,
        DS_RENAME,
        DS_DISPATCH,
        DS_SCHEDULE_R,
        DS_SCHEDULE_W,
        DS_WAITING_UNIT,
        DS_READY_SIG,
        DS_WAKEUP,
        DS_SELECT,
        DS_ISSUE_PRE,
        DS_ISSUE,
        DS_EXECUTE,
        DS_WRITEBACK,
        DS_COMMITTABLE,
        DS_COMMIT,
        DS_RETIRE,

        DS_STALL,
        DS_FLUSH,
        DS_RESCHEDULE,
        DS_RESC_REISSUE_EVENT,
        DS_RESC_REISSUE_FINISH,
        DS_RESC_LPREDMISS,
        DS_BRANCH_PREDICTION_MISS,
        DS_LATENCY_PREDICTION_MISS,
        DS_ADDRESS_PREDICTION_MISS,
        DS_LATENCY_PREDICTION_UPDATE,

        DS_INVALID
    };

    // Lanes are used for print stages that are independent each other.
    // For example, normal pipeline stages and stall states are managed 
    // independently, and stall states are overlayed on normal stages.
    // Users can overlay user defined stages on them by calling
    // PrintRawOutput with DL_USER_0 or a later constant.
    enum DumpLane
    {
        DL_OP    = 0,       // Reserved for dumping normal pipeline stages.
        DL_STALL = 1,       // Reserved for dumping stall states.
        DL_USER_0,
        DL_USER_1,
        DL_USER_2,
        DL_USER_3
    };

    // Users can print user defined dependency arrows by calling 
    // PrintOpDependency with DDT_USER_0 or a later constant.
    enum DumpDependency
    {
        DDT_WAKEUP = 0,     // Reserved for dumping wakeup.
        DDT_USER_0,
        DDT_USER_1,
        DDT_USER_2,
    };


    static const char* g_traceDumperStrTbl[] = 
    {
        "fetch          ",  // DS_FETCH
        "rename         ",  // DS_RENAME
        "dispatch       ",  // DS_DISPATCH
        "schedule(r)    ",  // DS_SCHEDULE_R
        "schedule(w)    ",  // DS_SCHEDULE_W
        "waiting unit   ",  // DS_WAITING_UNIT
        "ready sig      ",  // DS_READY_SIG
        "wakeup         ",  // DS_WAKEUP
        "select         ",  // DS_SELECT
        "issue pre      ",  // DS_ISSUE_PRE
        "issue          ",  // DS_ISSUE
        "execute        ",  // DS_EXECUTE
        "writeback      ",  // DS_WRITEBACK
        "committable   ",   // DS_COMMITTABLE
        "commit         ",  // DS_COMMIT
        "retire         ",  // DS_RETIRE

        "stall          ",  // DS_STALL
        "flush          ",  // DS_FLUSH
        "reschedule     ",  // DS_RESCHEDULE
        "rsc event      ",  // DS_RESC_REISSUE_EVENT
        "rsc finish     ",  // DS_RESC_REISSUE_FINISH
        "rsc ltpred miss",  // DS_RESC_LPREDMISS
        "br  pred miss  ",  // DS_BRANCH_PREDICTION_MISS
        "lat pred miss  ",  // DS_LATENCY_PREDICTION_MISS
        "adr pred miss  ",  // DS_ADDRESS_PREDICTION_MISS
        "lat pred update",  // DS_LATENCY_PREDICTION_UPDATE

    };

    static const char* g_visualizerDumperStrTbl[] = 
    {
        "F",    // DS_FETCH
        "Rn",   // DS_RENAME
        "D",    // DS_DISPATCH
        "Sr",   // DS_SCHEDULE_R
        "Sw",   // DS_SCHEDULE_W
        "Wat",  // DS_WAITING_UNIT
        "rs",   // DS_READY_SIG
        "Wku",  // DS_WAKEUP
        "Slc",  // DS_SELECT
        "Ip",   // DS_ISSUE_PRE
        "I",    // DS_ISSUE
        "X",    // DS_EXECUTE
        "Wb",   // DS_WRITEBACK
        "f",    // DS_COMMITTABLE
        "Cm",   // DS_COMMIT
        "Rt",   // DS_RETIRE

        "stl",  // DS_STALL
        "fls",  // DS_FLUSH
        "rsc",  // DS_RESCHEDULE
        "rse",  // DS_RESC_REISSUE_EVENT
        "rsf",  // DS_RESC_REISSUE_FINISH
        "rsl",  // DS_RESC_LPREDMISS
        "Xbm",  // DS_BRANCH_PREDICTION_MISS
        "Xlm",  // DS_LATENCY_PREDICTION_MISS
        "Xam",  // DS_ADDRESS_PREDICTION_MISS
        "Xlu",  // DS_LATENCY_PREDICTION_UPDATE
    };

    inline const char* GetTraceDumperStr( DUMP_STATE state )
    {
        return g_traceDumperStrTbl[ state ];
    }

    inline const char* GetVisualizerDumperStr( DUMP_STATE state )
    {
        return g_visualizerDumperStrTbl[ state ];
    }
}


#endif // SIM_DUMPER_DUMP_STATE_H

