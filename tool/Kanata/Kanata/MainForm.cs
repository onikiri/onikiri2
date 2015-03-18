// <TODO>
// ソースがカオス
// ズームアウトがかなり適当
// ステージが衝突したらへんな色で
// </TODO>

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

// Kanataファイル
// long[65536] : root index
// ... : body
// LogInfo
// long Position of LogInfo
// [EOF]


namespace Kanata
{
    public partial class MainForm : Form
    {

        // scrolling
        [DllImport("user32.dll")]
        private static extern int ScrollWindowEx(IntPtr hWnd, int dx, int dy, IntPtr prcScroll,
            IntPtr prcClip, IntPtr hrgnUpdate, IntPtr prcUpdate, uint flags);
        private const uint SW_INVALIDATE = 0x0002;

        /// <summary>
        ///  Insnのスクロールバーがスクロールする単位
        /// </summary>
        private int insnBarScrollUnit = 100;
        private Point panelMainView_lastScrollPosition;

		public int InsnBarScrollUnit
		{
			get { return insnBarScrollUnit; }
		}

		
		// ドラッグでの表示範囲移動
        private bool panelDragging;
		private Point leftMouseDownPos;
		private Point rightMouseDownPos;
		private Point lastMousePos;

        private bool ZoomedOut
        {
            get { return ZoomFactor < 0; }
        }

        public MainForm()
        {
            InitializeComponent();
            Initialize();
            // MouseWheelEvent must be set only once but
            // Initialize() may be called more than once so
            // the expression is not in Initialize() but 
            // in the constructor.
            this.MouseWheel += new MouseEventHandler(MainForm_MouseWheel);
        }

        // 全体の初期化
        private void Initialize()
        {
			InitializeConfiguration();
			InitializeView();
			InitializeLogic();

			panelMainView.Size = new Size( 0, 0 );
			panelLabel.Size = new Size( 0, 0 );

			coordinateSystem.SetInsnRange( 0, 0 );

            ApplyViewSetting( currentViewSetting );
            UpdateMenuItems();
        }

        private void ApplyViewSetting(ViewSetting vs)
        {
            ViewSetting oldViewSetting = currentViewSetting;
			SizeF lastGridSize = coordinateSystem.Grid;
			currentViewSetting = vs;


			// Calculate grid and cell sizes.
			if( config.RetainOriginalGridSize ) {
				coordinateSystem.SetGrid( currentViewSetting.CellSize );
				int segmentCount = 0;
				for( int i = 0; i < config.DrawInSeparateLane.Count; i++ ) {
					if( i < loginfo.SegmentCount && config.DrawInSeparateLane[ i ] ) {
						segmentCount++;
					}
				}
				if( segmentCount == 0 )
					segmentCount = 1;
				SizeF cellSize = currentViewSetting.CellSize;
				cellSize.Height = Math.Max( cellSize.Height / segmentCount, 1 );
				coordinateSystem.SetCell( cellSize );
			}
			else {
				coordinateSystem.SetCell( currentViewSetting.CellSize );
				SizeF grid = new SizeF( coordinateSystem.Cell.Width, 0 );
				for( int i = 0; i < config.DrawInSeparateLane.Count; i++ ) {
					if( i < loginfo.SegmentCount && config.DrawInSeparateLane[ i ] ) {
						grid.Height += coordinateSystem.Cell.Height;
					}
				}
				coordinateSystem.SetGrid( grid );
			}

            // グラデーションの基準サイズが変化するため，ブラシを作り直す
            MakeStageBrushes();
			MakeDependencyArrow();

            UpdateView();

            Point p;

            p = panelMainView.ScrollPosition;
			p.X = (int)( p.X * coordinateSystem.Grid.Width / lastGridSize.Width );
			p.Y = (int)( p.Y * coordinateSystem.Grid.Height / lastGridSize.Height );
            panelMainView.ScrollPosition = p;

            splitContainer1.Invalidate(true);

            p = splitContainer1.Panel2.AutoScrollPosition;
			p.X = (int)( -p.X * coordinateSystem.Grid.Width / oldViewSetting.CellSize.Width );
			p.Y = (int)( -p.Y * coordinateSystem.Grid.Width / oldViewSetting.CellSize.Width );
            splitContainer1.Panel2.AutoScrollPosition = p;

        }

        private void UpdateCaption()
        {
            this.Text = string.Format(
                "{0} - {1} - View: [{2},{3}] (cycle [{4},{5}]) - Total: [{6},{7}]",
                Application.ProductName,
                currentFile,
                coordinateSystem.IdFrom,
				coordinateSystem.IdTo,
				coordinateSystem.CycleFrom,
				coordinateSystem.CycleTo,
                loginfo.MinInsnId,
                loginfo.MaxInsnId);
        }

        private void UpdateView()
        {
			Size viewSize = coordinateSystem.ViewSize;
			int height = viewSize.Height + panelMainView.Size.Height - 1;
			int width = viewSize.Width + panelMainView.Size.Width - 1;
            panelMainView.VirtualSize = new Size(width, height);
            panelMainView.VScrollLargeChange = panelMainView.Size.Height;
            panelMainView.HScrollLargeChange = panelMainView.Size.Width;
            panelMainView.VScrollSmallChange = 32;
            panelMainView.HScrollSmallChange = 32;

			MakeStageBrushes();
			
			if( config.MainViewTransparent ) {
                // Opacityを1.0以外に設定する場合は、0か255を要素とする色のみTransparencyKeyとして使用できる？
				Color c = Color.FromArgb( 255, 255, 254, 255 );
                TransparencyKey = c;
                panelMainView.BackColor = c;
            }
            else {
                TransparencyKey = Color.Empty;
                panelMainView.BackColor = Color.FromArgb((int)config.MainViewBackColorARGB);
            }

			Invalidate( true );
		}


		private bool IsVisibleInsnId( ulong id )
		{
			ulong maxVisibleInsnID = Math.Min( loginfo.MaxInsnId, coordinateSystem.IdTo );
			if( maxVisibleInsnID == 0 )
				return false;
			return id <= Math.Min( loginfo.MaxInsnId, coordinateSystem.IdTo );
		}

		private bool IsYVisible( int y )
		{
			ulong maxVisibleInsnID = Math.Min( loginfo.MaxInsnId, coordinateSystem.IdTo );
			if( maxVisibleInsnID == 0 ) {
				return false;
			}
			return coordinateSystem.InsnIdAtY_NoClip( y ) <= maxVisibleInsnID;
		}

		private int GetVisibleBottom()
		{
			ulong id = Math.Min( loginfo.MaxInsnId, coordinateSystem.IdTo );
			if( id == 0 ) {
				return 0;
			}
			return coordinateSystem.YAtInsnId( id ) + (int)coordinateSystem.Grid.Height;
		}

		//
        // Event Handlers
        //

        void MainForm_MouseWheel(object sender, MouseEventArgs e)
		{
			if( Control.ModifierKeys == Keys.Control ) {
				// Zoom in/out if control key is down

				
				// Translate a mouse point relative to panelMainView.
				Point panelMousePoint = panelMainView.PointToClient( Cursor.Position );
				MouseEventArgs panelMouse = 
					new MouseEventArgs(
						e.Button, e.Clicks,
						panelMousePoint.X,
						panelMousePoint.Y,
						e.Delta 
				);
				panelMouse = panelMainView.TranslateMouseEventArgs( panelMouse );

				ChangeZoom(
					new Point( panelMouse.X, panelMouse.Y ),
					ZoomFactor + ( e.Delta > 0 ? 1 : -1 ), 
					true,
					true
				);
			}
			else{
				// Scroll
				int nLines = e.Delta * SystemInformation.MouseWheelScrollLines / -120;

				Point p = panelMainView.ScrollPosition;
				p.Y = p.Y + (int)viewSettings[viewSettings.Count - 1].CellSize.Height * nLines;
				if( AdjustXOnScroll ) {
					p.X += CalculateInsnXDiffByY( panelMainView.ScrollPosition.Y, p.Y );
				}
				panelMainView.ScrollPosition = p;
			}
		}

        private void ExitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void OpenToolStripMenuItem_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();

            ofd.InitialDirectory = Environment.CurrentDirectory;
            ofd.CheckFileExists = true;
            ofd.CheckPathExists = true;
            ofd.DefaultExt = ".kanata";

            ofd.Filter = "Kanata Files (*.kanata)|*.kanata";
            ofd.FilterIndex = 1;

            if (ofd.ShowDialog() == DialogResult.OK) {
                OpenFile(ofd.FileName);
            }
        }

        private void panelMainView_MouseDown(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left) {
                panelMainView.Cursor = Cursors.Hand;
                panelDragging = true;
                leftMouseDownPos = new Point(e.X, e.Y);
                toolTip.Hide(panelMainView);
            }
			else if(e.Button == MouseButtons.Right){
                rightMouseDownPos = new Point(e.X, e.Y);
			}
        }

        private void panelMainView_MouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left) {
                panelMainView.Cursor = Cursors.Default;
                panelDragging = false;
            }
        }
        
        private void panelMainView_MouseMove(object sender, MouseEventArgs e)
        {
            Point mousePos = new Point(e.X, e.Y);
            if (lastMousePos == mousePos)
                return;
            lastMousePos = mousePos;
            if (panelDragging) {
                Point p = panelMainView.ScrollPosition;
                p.Y = p.Y - (e.Y - leftMouseDownPos.Y);
                p.X = p.X - (e.X - leftMouseDownPos.X);
                panelMainView.ScrollPosition = p;
//                Debug.WriteLine(string.Format("({0}, {1})", e.X - lastMousePos.X, e.Y - lastMousePos.Y));
            }
            else {
				long cycle = coordinateSystem.CycleAtX( e.X ) - coordinateSystem.CycleFrom;
				ulong id = coordinateSystem.InsnIdAtY( e.Y ) - coordinateSystem.IdFrom;
				if( !IsVisibleInsnId( id ) ) {
					toolTip.SetToolTip( panelMainView, "" );
					return;
				}

                // マウスオーバー位置のInsnのステージを表示する
				string stageName = "";
				string stageComment = "";
				Insn insn = GetInsn( id );
                if (insn != null){

					for( int segmentID = 0; segmentID < insn.StageSegments.Count; segmentID++ ) {
						List<Insn.Stage> stages = insn.StageSegments[segmentID];
						for( int i = 0; i < stages.Count; i++ ) {
							Insn.Stage stage = stages[i];
							long beginCycle = insn.StartCycle + stage.BeginRelCycle - coordinateSystem.CycleFrom;
							long length = (int)stage.Length;
							if( (beginCycle <= cycle && cycle < beginCycle + length) ||
								(beginCycle == cycle && length == 0) ) {
								if( stageName.Length != 0 ) {
									stageName += ", ";
								}
								stageName += loginfo.StageNames[ segmentID ][ stage.Id ] + "[" + length + "]";
								if( stage.Comment != null ) {
									stageComment += stage.Comment;
								}
							}
						}
					}
					string tipStr = string.Format( "(+{0},+{1}) {2}\n{3}", cycle, id, stageName, stageComment );
					toolTip.SetToolTip( panelMainView, tipStr );
				}

            }
        }

        private void panelMainView_Click(object sender, EventArgs e)
        {
            splitContainer1.Focus();
            this.Focus();
        }

        private void panelMainView_DoubleClick(object sender, EventArgs e)
        {
			// コントロールキーを押してる場合は縮小に
			if( Control.ModifierKeys == Keys.Control ) {
				ChangeZoom( leftMouseDownPos, trackBarZoom.Value - 1, true, true );
			}
			else {
				ChangeZoom( leftMouseDownPos, trackBarZoom.Value + 1, true, true );
			}
		}

		private void ChangeZoom( Point basePoint, int zoomValue, bool changeTrackBar, bool mousePointBaseX )
		{
			//Trace.WriteLine( string.Format( "({0}, {1})", basePoint.X, basePoint.Y ) );


			// 基準座標と拡大率
			if( zoomValue > trackBarZoom.Maximum ) {
				zoomValue = trackBarZoom.Maximum;
			}
			else if( zoomValue < trackBarZoom.Minimum ) {
				zoomValue = trackBarZoom.Minimum;
			}

			int baseY = basePoint.Y;
			int offsetY = (basePoint.Y - panelMainView.ScrollPosition.Y);
			int offsetX = (basePoint.X - panelMainView.ScrollPosition.X);

			// 対応するInsn を取得
			ulong id = coordinateSystem.InsnIdAtY( baseY );
			Insn insn = GetInsn( id );
			if( insn == null )
				return;

			long offsetCycle = coordinateSystem.CycleAtX( basePoint.X );

			// 拡大
			ZoomFactor = zoomValue;

			if( changeTrackBar ) {
				trackBarZoom.Value = zoomValue;
			}

			// 位置を直す
			int y = coordinateSystem.YAtInsnId( insn.Id ) - offsetY;
			int x = 
				mousePointBaseX  ?
				coordinateSystem.XAtCycle( offsetCycle ) - offsetX :
				coordinateSystem.XAtCycle( GetInsn( coordinateSystem.InsnIdAtY( y ) ).StartCycle );

			panelMainView.ScrollPosition = new Point( x, y );
		}

        private void MainForm_DragEnter(object sender, DragEventArgs e)
        {
            e.Effect = DragDropEffects.None;
            if (e.Data.GetDataPresent(DataFormats.FileDrop)) {
                string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);
                if (files.Length == 1)
                    e.Effect = DragDropEffects.Copy;
            }
        }


		private delegate void DelegateDropFile( string fileName );
		private void MainForm_DragDrop( object sender, DragEventArgs e )
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop)) {
                string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);

				// Opening a file is delayed because another application 
				// from which a file is dropped freezes if this method does not 
				// immediately return.
				var delegateOpenFile = new DelegateDropFile( OpenFile );
				BeginInvoke( delegateOpenFile, new Object[] { files[0] } );
				
				// If drop success, focuses on this form.
				Activate();
            }
        }


        
        // 本体の描画を行うイベントハンドラ
        // DrawInsnを呼び出す
        private void panelMainView_Paint(object sender, PaintEventArgs e)
        {
            Graphics g = e.Graphics;
            Rectangle rect = e.ClipRectangle;

            int dy =
				panelMainView.ScrollPosition.Y -
				panelMainView_lastScrollPosition.Y;
            ScrollWindowEx(
				panelLabel.Handle, 
				0, -dy,
                IntPtr.Zero,
				IntPtr.Zero,
				IntPtr.Zero,
				IntPtr.Zero, 
				SW_INVALIDATE
			);
            panelLabel.Update();
            panelMainView_lastScrollPosition = panelMainView.ScrollPosition;
			DrawMainView( g, rect );
        }


		// ラベル上でマウスオーバー位置のInsnの情報を表示する
		private void panelLabel_MouseMove( object sender, MouseEventArgs e )
		{
			Point mousePos = new Point( e.X, e.Y );
			if( lastMousePos == mousePos )
				return;
			lastMousePos = mousePos;

			ulong id = coordinateSystem.InsnIdAtY( e.Y + panelMainView.ScrollPosition.Y ) - coordinateSystem.IdFrom;
			if( !IsVisibleInsnId( id ) ) {
				toolTip.SetToolTip( panelLabel, "" );
				return;
			}

			Insn insn = GetInsn( id );
			if( insn != null ) {
				String msg = String.Format( 
					"{0}\n" +
					"{1}\n" +
					"Line :\t\t{2}\n" +
					"Global Serial ID:\t{3}\n" +
					"Thread ID:\t\t{4}\n" +
					"Retire ID:\t\t{5}\n",

					insn.Name,
					insn.Detail,
					insn.Id,
					insn.Gsid,
					insn.Tid,
					insn.Rid
				);
				if( insn.Flushed ) {
					msg += "# This op is flushed.";
				}
				toolTip.SetToolTip( panelLabel, msg );
			}

		}

		// ラベルの表示
		private void panelLabel_Paint( object sender, PaintEventArgs e )
        {
            if (!currentViewSetting.DrawInsnName)
                return;

            Graphics g = e.Graphics;
            Rectangle rect = e.ClipRectangle;
			DrawLabelView( g, rect );
        }

		private void trackBarZoom_MouseUp( object sender, MouseEventArgs e )
		{
			// 拡大率を変えた後は，メインビューにフォーカスを移す
			// 拡大率を変えた直後にマウスホイールを回した場合に，そのまま
			// 拡大率が変わってしまうのを防止するため
			panelMainView.Focus();
		}

		private void trackBarZoom_ValueChanged( object sender, EventArgs e )
        {
			ChangeZoom( panelMainView.ScrollPosition, trackBarZoom.Value, false, false );
        }

        private void trackBarZoom_MouseMove(object sender, MouseEventArgs e)
        {
            int zoom = trackBarZoom.Value;

            if (zoom < 0) {
                toolTip.SetToolTip(trackBarZoom, string.Format("/{0}", Math.Pow(2.0, -zoom)));
            }
            else {
                toolTip.SetToolTip(trackBarZoom, string.Format("{0}", trackBarZoom.Value));
            }
        }


        private void panelLabel_MouseClick(object sender, MouseEventArgs e)
        {
            int y = e.Y + panelMainView.ScrollPosition.Y;
			ulong id = coordinateSystem.InsnIdAtY( y );

            Insn insn = GetInsn(id);
            if (insn == null)
                return;

			int x = coordinateSystem.XAtCycle( insn.StartCycle );

            panelMainView.ScrollPosition = new Point(x, panelMainView.ScrollPosition.Y);
        }

        private void OpacityToolStripMenuItem_Click(object sender, EventArgs e)
        {
            this.Opacity = double.Parse((string)((ToolStripMenuItem)sender).Tag);
            Invalidate(true);
        }

        private void MainForm_FormClosed(object sender, FormClosedEventArgs e)
        {
            WriteViewSettings();
			WriteConfiguration();
        }

        private void DrawDependencyLeftCurveToolStripMenuItem1_Click(object sender, EventArgs e)
        {
            DrawDependencyLeftCurve = !DrawDependencyLeftCurve;
            UpdateMenuItems();
            Invalidate(true);
        }

        private void DrawDependencyInsideCurveToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DrawDependencyInsideCurve = !DrawDependencyInsideCurve;
            UpdateMenuItems();
            Invalidate(true);
        }

		private void DrawDependencyInsideLineToolStripMenuItem_Click( object sender, EventArgs e )
		{
			DrawDependencyInsideLine = !DrawDependencyInsideLine;
			UpdateMenuItems();
			Invalidate( true );
		}
		
		private void UpdateMenuItems()
        {
            DrawDependencyLeftCurveToolStripMenuItem.Checked = DrawDependencyLeftCurve;
			DrawDependencyInsideCurveToolStripMenuItem.Checked = DrawDependencyInsideCurve;
			DrawDependencyInsideLineToolStripMenuItem.Checked = DrawDependencyInsideLine;
			TransparentToolStripMenuItem.Checked = config.MainViewTransparent;
            AdjustXOnScrollToolStripMenuItem.Checked = AdjustXOnScroll;

            CSDefaultMenuItem.Checked = config.ColorScheme == ColorScheme.Default;
            CSBlueMenuItem.Checked = config.ColorScheme == ColorScheme.Blue;
            CSRedMenuItem.Checked = config.ColorScheme == ColorScheme.Red;

			var stageLaneMenuItems = new[]
			{
				StageLane1ToolStripMenuItem,
				StageLane2ToolStripMenuItem,
				StageLane3ToolStripMenuItem,
				StageLane4ToolStripMenuItem
			};

			for( int i = 0; i < 4; i++ ) {
				int laneIndex = i + 1;
				var menu = stageLaneMenuItems[ i ];
				if( laneIndex < loginfo.SegmentCount ){
					menu.Enabled = true;
					menu.Checked = config.DrawInSeparateLane[ laneIndex ];
				}
				else{
					menu.Enabled = false;
				}
			}
		
			RetainHeightToolStripMenuItem.Checked = config.RetainOriginalGridSize;
		}

        private void TransparentToolStripMenuItem_Click(object sender, EventArgs e)
        {
            config.MainViewTransparent = !config.MainViewTransparent;
            UpdateMenuItems();
			UpdateView();
        }

        // スクロール時，スクロール前に一番上だった命令が表示されていたX座標に，スクロール後の命令も表示する
        private void panelMainView_Scroll(object sender, ScrollEventArgs e)
        {
            if (e.ScrollOrientation == ScrollOrientation.VerticalScroll && AdjustXOnScroll)
            {
                panelMainView.ScrollPosition = new Point(
                    panelMainView.ScrollPosition.X + CalculateInsnXDiffByY(e.OldValue, e.NewValue),
                    panelMainView.ScrollPosition.Y
                );
            }
        }

        // oldY, newY の位置にあるInsnのX座標の差を得る
        private int CalculateInsnXDiffByY(int oldY, int newY)
        {
			Insn oldTop = GetInsn( coordinateSystem.InsnIdAtY( oldY ) );
			Insn newTop = GetInsn( coordinateSystem.InsnIdAtY( newY ) );
            if (oldTop == null || newTop == null)
                return 0;

			return coordinateSystem.XAtCycle( newTop.StartCycle ) - coordinateSystem.XAtCycle( oldTop.StartCycle );
        }

        // X 
        private void AdjustXOnScrollToolStripMenuItem_Click(object sender, EventArgs e)
        {
            AdjustXOnScroll = !AdjustXOnScroll;
            UpdateMenuItems();
            Invalidate(true);           
        }

        // 再読み込み
        private void ReloadToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (currentFile != null)
                OpenFile(currentFile);
        }

        // バージョン情報
        private void AboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            VersionForm version = new VersionForm();

            // メインフォームの位置に合わせる
            version.StartPosition = FormStartPosition.Manual;
            version.Location = new Point(
                Left + (Width - version.Width) / 2,
                Top + (Height - version.Height) / 2
            );

            version.ShowDialog();
        }

		// Clicked on color scheme menu items.
        private void CSMenuItem_Click(object sender, EventArgs e)
        {
			if( sender.Equals( CSDefaultMenuItem ) ) {
				config.ColorScheme = ColorScheme.Default;
			}
			else if( sender.Equals( CSRedMenuItem ) ){
				config.ColorScheme = ColorScheme.Red;
			}
			else {
				config.ColorScheme = ColorScheme.Blue;
			}
			UpdateMenuItems();
            MakeStageBrushes();
			ApplyViewSetting( currentViewSetting );
			UpdateView();
        }

		// Clicked on option menu items about stage lanes.
		private void StageLaneToolStripMenuItem_Click( object sender, EventArgs e )
		{
			var menuItems = new[]
			{
				StageLane1ToolStripMenuItem,
				StageLane2ToolStripMenuItem,
				StageLane3ToolStripMenuItem,
				StageLane4ToolStripMenuItem
			};

			int clicked = 0;
			for( int i = 0; i < 4; i++ ) {
				if( sender.Equals( menuItems[i] ) ) {
					clicked = i + 1;
				}
			}
			if( clicked == 0 ) {
				throw new ApplicationException( "An unknown menu item is clicked." );
			}
			config.DrawInSeparateLane[ clicked ] = !config.DrawInSeparateLane[clicked];
			UpdateMenuItems();
			ApplyViewSetting( currentViewSetting );
			UpdateView();
		}

		private void RetainHeightToolStripMenuItem_Click( object sender, EventArgs e )
		{
			config.RetainOriginalGridSize = !config.RetainOriginalGridSize;
			UpdateMenuItems();
			ApplyViewSetting( currentViewSetting );
			UpdateView();
		}

		// On this window is activated.
		protected override void OnActivated( EventArgs e )
        {
            base.OnActivated(e);
			CheckReload();
			splitContainer1.Focus();
		}

		// コンテキストメニュー
		private void contextMenuStrip_Opened( object sender, EventArgs e )
		{
			zoomInToolStripMenuItem.Enabled = loaded && (ZoomFactor < 3);
			zoomOutToolStripMenuItem.Enabled = loaded && (ZoomFactor > -11);
			viewThisPointToolStripMenuItem.Enabled = loaded;

		}

		private void zoomInToolStripMenuItem_Click( object sender, EventArgs e )
		{
			ChangeZoom( rightMouseDownPos, ZoomFactor + 1, true, true );
		}

		private void zoomOutToolStripMenuItem_Click( object sender, EventArgs e )
		{
			ChangeZoom( rightMouseDownPos, ZoomFactor - 1, true, true );
		}

		private void viewThisPointToolStripMenuItem_Click( object sender, EventArgs e )
		{
			ChangeZoom( rightMouseDownPos, ZoomFactor, true, false );
		}

    }
}