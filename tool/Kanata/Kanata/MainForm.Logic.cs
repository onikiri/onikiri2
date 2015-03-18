using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Collections.ObjectModel;
using System.Xml.Serialization;

namespace Kanata
{
    partial class MainForm
    {
		private bool   loaded;
		private string loadedFile;
		private DateTime lastReloadCheckedTime;

		private LogLoader loader = null;
		private LogInfo loginfo = new LogInfo();
		private string currentFile = null;
		private Insn[] insns = null;

		private DependencyRange[] dependencyRanges = null;

		// A coordinate system in a view.
		ViewCoordinateSystem coordinateSystem = new ViewCoordinateSystem();

		struct ExecStageInfo
		{
			public Int32 lane;
			public Int32 id;
		};
		private List<ExecStageInfo> execStageId = new List<ExecStageInfo>();
		private List<ExecStageInfo> ExecStageId
		{
			get { return execStageId; }
		}

		private bool IsExecStageId( Int32 lane, Int32 id )
		{
			foreach( var i in ExecStageId ) {
				if( i.id == id && i.lane == lane ) {
					return true;
				}
			}
			return false;
		}

		// insn のID にかかっている依存関係の範囲
		public class DependencyRange
		{
			protected ulong front;
			protected ulong back;

			public DependencyRange( ulong front, ulong back )
			{
				this.front = front;
				this.back = back;
			}

			public void SetRange( ulong front, ulong back )
			{
				if( this.front > front ) {
					this.front = front;
				}
				if( this.back < back ) {
					this.back = back;
				}
			}

			public ulong GetFront()
			{
				return front;
			}

			public ulong GetBack()
			{
				return back;
			}
		};
		
		private void InitializeLogic()
        {
            loaded = false;
        }
		
		private void LoadKanataLogFile( string fileName )
		{
			loader = new LogLoader();
			
			//loader.Load( fileName );
			LoadingForm lodingForm = new LoadingForm( loader, fileName );
			lodingForm.ShowDialog( this );

			if( !loader.SuccessfullyLoaded ) {
				throw new ApplicationException( "ファイルの読み込みに失敗しました" );
			}

			loginfo = loader.LogInfo;
			ulong idFrom = loginfo.MinInsnId;
			coordinateSystem.SetInsnRange( idFrom, idFrom + (ulong)coordinateSystem.ViewInsnCount - 1 );

			LoadInsns();

		}

		private void OpenFile( string fileName )
        {
            currentFile = fileName;
            try {
				// Finalize loaded data.
				CloseDB();
				WriteConfiguration();
				WriteViewSettings();
				ExecStageId.Clear();

				// Initialize
				Initialize();

				// Load a log file.
				LoadKanataLogFile( fileName );

				ApplyViewSetting( currentViewSetting );
				MakeStageBrushes();
				UpdateCaption();
				UpdateView();
				splitContainer1.Panel2.AutoScrollPosition = new Point( 0, 0 );
				Invalidate( true );

				loadedFile = fileName;
				loaded = true;
				lastReloadCheckedTime = File.GetLastWriteTime( fileName );
			}
            catch (Exception e)
            {
                loaded = false;
                MessageBox.Show( e.Message, "エラー" );
            }
        }

        private void ReloadFile()
        {
            if(loaded){
                OpenFile(loadedFile);
            }
        }

        private void CheckReload()
        {
            if( !loaded )
                return;

            DateTime currentTime = File.GetLastWriteTime( loadedFile );
			if( currentTime > lastReloadCheckedTime ) {
				lastReloadCheckedTime = currentTime;
                if( MessageBox.Show(
                        loadedFile + " は更新されています．\n再読み込みを行いますか？",
                        "確認",
                        MessageBoxButtons.OKCancel
                    ) == DialogResult.Cancel
                ){
					return;
                }

				ReloadFile();
            }

        }

        private void CloseDB()
        {
            if (loader != null)
            {
				loader = null;
            }
        }

		// loader からデータを読みだして配列を作成、MainForm::insnsに置き換え
        // この時AddConsumerしている
        private void LoadInsns()
        {
            if (loader == null)
                return;


			coordinateSystem.SetInsnRange( loginfo.MinInsnId, loginfo.MaxInsnId );
            insns = new Insn[ coordinateSystem.ViewInsnCount ];
			dependencyRanges = new DependencyRange[ coordinateSystem.ViewInsnCount ];

            long cycleFrom = long.MaxValue;
			long cycleTo = long.MinValue;
			ulong idFrom = coordinateSystem.IdFrom;
			ulong idTo = coordinateSystem.IdTo;

			for( ulong id = idFrom; id <= idTo; id++ )
            {
                Insn insn = null;

                try
                {
                    //insn = db.Read(id);
					insn = loader.GetInsnFromDB( id );
                }
                catch (ArgumentOutOfRangeException)
                {
                    insn = null;
                }
				insns[ id - idFrom ] = insn;
				dependencyRanges[ id - idFrom ] = new DependencyRange( id, id );
                if (insn == null)
                    continue;
                if (cycleFrom > insn.StartCycle)
                    cycleFrom = insn.StartCycle;
                if (cycleTo < insn.EndCycle)
                    cycleTo = insn.EndCycle;
            }

            if (cycleFrom == long.MaxValue)
                cycleTo = cycleFrom = 0;

			coordinateSystem.SetCycleRange( cycleFrom, cycleTo );
			
			// Extract dependencies.
			foreach (Insn consinsn in insns)
            {
                if (consinsn == null)
                    continue;
                
				foreach (Insn.Relation r in consinsn.Producers)
                {
                    Insn prodinsn = GetInsn(r.id);
                    if (prodinsn != null)
                    {
						Insn.Relation rel = new Insn.Relation();
						rel.id = consinsn.Id;
						rel.type = r.type;
                        prodinsn.AddConsumer(rel);

						// i を横切る依存関係のうち最大と最小の範囲を登録
						ulong front = Math.Min( r.id, consinsn.Id );
						ulong back  = Math.Max( r.id, consinsn.Id );
						for( ulong i = front; i <= back; i++ ) {
							dependencyRanges[ i - idFrom ].SetRange( front, back );
						}
					}
                }
            }

			// Extract execution stages.
			// Stage names including "X" are regarded as execution stages.
			for( int i = 0; i < loginfo.SegmentCount; ++i ) {
				foreach( var stageName in loginfo.StageNames[ i ] ) {
					if( stageName.Contains( "X" ) ) {
						int id = loginfo.GetStageId( i, stageName );
						if( id >= 0 ) {
							ExecStageInfo info;
							info.lane = i;
							info.id = id;
							ExecStageId.Add( info );
						}
					}
				}
			}
		}

        //idはinsn.Id(SerialId)
        private Insn GetInsn(ulong id)
        {
            if (id < coordinateSystem.IdFrom || coordinateSystem.IdTo < id)
                return null;
            else
				return insns[ id - coordinateSystem.IdFrom ];
        }

		private DependencyRange GetDependencyRange( ulong id )
		{
			ulong from = Math.Max( loginfo.MinInsnId, coordinateSystem.IdFrom );
			ulong to   = Math.Min( loginfo.MaxInsnId, coordinateSystem.IdTo );
			if( id < from )
				id = from;
			if( id > to )
				id = to;
			return dependencyRanges[ id - coordinateSystem.IdFrom ];
		}
	}
}
