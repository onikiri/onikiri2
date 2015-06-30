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


#ifndef SIM_DUMPER_VISUALIZATION_DUMPER_H
#define SIM_DUMPER_VISUALIZATION_DUMPER_H

#include <fstream>
#include <string>

#include "Utility/String.h"

#include "Sim/Op/OpArray/OpArray.h"
#include "Sim/Op/OpContainer/OpExtraStateTable.h"
#include "Sim/Foundation/Resource/ResourceNode.h"

#include "Env/Param/ParamExchange.h"
#include "Sim/Dumper/DumpState.h"

namespace Onikiri
{
    class Core;
    class Thread;

    // simulationの結果をVisualizeするためのクラス
    class VisualizationDumper : public ParamExchange
    {
    public:
        // parameter mapping
        BEGIN_PARAM_MAP( "/Session/Environment/Dumper/VisualizationDumper/" )
            PARAM_ENTRY( "@FileName",   m_visFileName )
            PARAM_ENTRY( "@EnableDump", m_enabled )
            PARAM_ENTRY( "@EnableGzip", m_gzipEnabled )
            PARAM_ENTRY( "@GzipLevel",  m_gzipLevel )
            PARAM_ENTRY( "@SkipInsns",  m_skipInsns )
        END_PARAM_MAP()

        // constructor/destructor
        VisualizationDumper();
        ~VisualizationDumper();

        void Initialize( const String& suffix, PhysicalResourceArray<Core>& coreList );
        void Finalize();
        bool IsEnabled();

        // VisDumpを書きだすために外から呼ばれる関数

        // VisualizationDumperに現在のcycle/insn数を教えるための関数
        void SetCurrentCycle( const s64 cycle );
        void SetCurrentInsnCount( Thread* thread, const s64 count );

        // opの現在のState(PipelineStage/Retire/Flushなど)を表示する
        void PrintOpState( OpIterator op, DUMP_STATE state );

        // Dump the begin/end of stall.
        void PrintStallBegin( OpIterator op );
        void PrintStallEnd  ( OpIterator op );

        // Print dependency arrows from producerOp to consumerOp.
        // This method is typically used for printing wakeup.
        void PrintOpDependency(
            const OpIterator producerOp, 
            const OpIterator consumerOp, 
            DumpDependency type 
        );

        // Print user defined stages. 
        // See comments for DumpLane.
        //   op:    Dumped op.
        //   begin: The stage is begun or not.
        //   stage: User defined stage string.
        //   lane:  A lane of the stage.
        void PrintRawOutput( OpIterator op, bool begin, const char* stage, DumpLane lane );

    private:
        
        static const u64 INVALID_VIS_SERIAL = ~((u64)0);
        
        // Each op has OpState, a extra state for dump.
        struct OpState 
        {
            DUMP_STATE dumpState;   // Opの最後のState
            u64 visSerialID;        // 出力ファイル内でのシリアル
            bool inPipeline;        // （Kanataで表示可能な範囲の）パイプライン中にいるかどうか
            bool stalled;           // Op is stalled or not.

            struct Stage
            {
                bool in;
                std::string name;
                Stage() : in(false)
                {
                }
            };
            std::map< DumpLane, Stage > lastRawStages;

            OpState();
        };

        // OpState can be retrieved from CoreState like the following:
        // GetOpState() or 
        // m_coreStateTable[ op->GetCore() ]->opState[ op ]
        struct CoreState
        {
            OpExtraStateTable<OpState> opState;
        };

        struct ThreadState
        {
            s64 retiredInsnCount;
        };
        
        // Each core/thread has extra information for dump.
        typedef unordered_map< Core*, CoreState >       CoreStateMap;
        typedef unordered_map< Thread*, ThreadState >   ThreadStateMap;
        CoreStateMap    m_coreStateTable; 
        ThreadStateMap  m_threadStateTable;

        OpState*   GetOpState( OpIterator op );
        DUMP_STATE GetLastState( OpIterator op );
        void SetLastState  ( OpIterator op, DUMP_STATE state );
        u64  GetVisSerialID( OpIterator op );
        void SetVisSerialID( OpIterator op, u64 visSerialID );
        
        s64 GetTotalRetiredInsnCount();

        bool m_enabled;     // VisDumpを書きだすかどうか
        bool m_gzipEnabled; // VisDumpをgzip圧縮するかどうか
        int  m_gzipLevel;   // gzipの圧縮レベル

        std::string m_visFileName; // visDumpを出力するファイル名
        boost::iostreams::filtering_ostream m_visStream;    // visDumpを出力するファイルストリーム

        s64 m_visCurrentCycle;  // シミュレータの現在の時刻
        s64 m_visLastPrintCycle;
        s64 m_skipInsns;        // The count of insns for skipping dump.

        u64 m_visSerialID;      // ログファイル内でのシリアルID

        // ダンプのスキップ判定
        bool IsDumpSkipped( OpIterator op );

        // DUMP_STATE <> 表示名変換
        const char* ToStringFromDumpState( DUMP_STATE state );

        // Print initialization of 'op'.
        // PrintOpInitLabel is only called from PrintOpInit.
        void PrintOpInit( const OpIterator op );
        void PrintOpInitLabel( const OpIterator op );

        // Print a label text.
        enum OpLabelType{
            POLT_LABEL  = 0,
            POLT_DETAIL = 1
        };
        void PrintOpLabel( const OpIterator op, OpLabelType type, const std::string& label );

        // 現在のサイクルを表示
        void PrintCycle();

        // opが前のPipelineStageから出たことを表示
        void PrintLastPipelineStage( const OpIterator op, DUMP_STATE lastState );

        // opがRetire/Flushされたことを表示
        void PrintOpEnd( const OpIterator op, DUMP_STATE lastState );
        void PrintOpEndLabel( const OpIterator op );

        // opが新しいPipelineStageに入ったことを表示
        void PrintNextPipelineStage( const OpIterator op, DUMP_STATE state );
        
        // Print stall states.
        void PrintStallBody( const OpIterator op, bool stall );
        void PrintStall( const OpIterator op, bool stall );

    };

};

#endif //SIM_DUMPER_VIS_DUMPER_H


