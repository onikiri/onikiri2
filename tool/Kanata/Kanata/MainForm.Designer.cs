namespace Kanata
{
    partial class MainForm
    {
        /// <summary>
        /// 必要なデザイナ変数です。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 使用中のリソースをすべてクリーンアップします。
        /// </summary>
        /// <param name="disposing">マネージ リソースが破棄される場合 true、破棄されない場合は false です。</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null)) {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows フォーム デザイナで生成されたコード

        /// <summary>
        /// デザイナ サポートに必要なメソッドです。このメソッドの内容を
        /// コード エディタで変更しないでください。
        /// </summary>
        private void InitializeComponent()
        {
			this.components = new System.ComponentModel.Container();
			this.splitContainer1 = new System.Windows.Forms.SplitContainer();
			this.panelLabel = new System.Windows.Forms.Panel();
			this.contextMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
			this.zoomInToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.zoomOutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.viewThisPointToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolTip = new System.Windows.Forms.ToolTip(this.components);
			this.FileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.OpenToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.ExitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.HelpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.AboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.mainMenuStrip = new System.Windows.Forms.MenuStrip();
			this.EditToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.ReloadToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.ViewToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.StageLaneRootToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.StageLane1ToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.StageLane2ToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.StageLane3ToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.StageLane4ToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
			this.RetainHeightToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.OpacityPopupToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.OpacityStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
			this.OpacityToolStripMenuItem2 = new System.Windows.Forms.ToolStripMenuItem();
			this.OpacityToolStripMenuItem3 = new System.Windows.Forms.ToolStripMenuItem();
			this.DependencyToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.DrawDependencyLeftCurveToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.DrawDependencyInsideCurveToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.DrawDependencyInsideLineToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.ColorSchemeMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.CSDefaultMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.CSRedMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.CSBlueMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.TransparentToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.AdjustXOnScrollToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.trackBarZoom = new System.Windows.Forms.TrackBar();
			this.panelMainView = new Kanata.LargePanel();
			this.splitContainer1.Panel1.SuspendLayout();
			this.splitContainer1.Panel2.SuspendLayout();
			this.splitContainer1.SuspendLayout();
			this.contextMenuStrip.SuspendLayout();
			this.mainMenuStrip.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.trackBarZoom)).BeginInit();
			this.SuspendLayout();
			// 
			// splitContainer1
			// 
			this.splitContainer1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.splitContainer1.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
			this.splitContainer1.Location = new System.Drawing.Point(12, 27);
			this.splitContainer1.Name = "splitContainer1";
			// 
			// splitContainer1.Panel1
			// 
			this.splitContainer1.Panel1.Controls.Add(this.panelLabel);
			// 
			// splitContainer1.Panel2
			// 
			this.splitContainer1.Panel2.BackColor = System.Drawing.SystemColors.Window;
			this.splitContainer1.Panel2.Controls.Add(this.panelMainView);
			this.splitContainer1.Size = new System.Drawing.Size(1028, 667);
			this.splitContainer1.SplitterDistance = 300;
			this.splitContainer1.TabIndex = 1;
			// 
			// panelLabel
			// 
			this.panelLabel.BackColor = System.Drawing.SystemColors.Window;
			this.panelLabel.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panelLabel.Location = new System.Drawing.Point(0, 0);
			this.panelLabel.Name = "panelLabel";
			this.panelLabel.Size = new System.Drawing.Size(300, 667);
			this.panelLabel.TabIndex = 2;
			this.panelLabel.Paint += new System.Windows.Forms.PaintEventHandler(this.panelLabel_Paint);
			this.panelLabel.MouseClick += new System.Windows.Forms.MouseEventHandler(this.panelLabel_MouseClick);
			this.panelLabel.MouseMove += new System.Windows.Forms.MouseEventHandler(this.panelLabel_MouseMove);
			// 
			// contextMenuStrip
			// 
			this.contextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.zoomInToolStripMenuItem,
            this.zoomOutToolStripMenuItem,
            this.viewThisPointToolStripMenuItem});
			this.contextMenuStrip.Name = "contextMenuStrip";
			this.contextMenuStrip.Size = new System.Drawing.Size(157, 70);
			this.contextMenuStrip.Opened += new System.EventHandler(this.contextMenuStrip_Opened);
			// 
			// zoomInToolStripMenuItem
			// 
			this.zoomInToolStripMenuItem.Name = "zoomInToolStripMenuItem";
			this.zoomInToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
			this.zoomInToolStripMenuItem.Text = "ズームイン";
			this.zoomInToolStripMenuItem.Click += new System.EventHandler(this.zoomInToolStripMenuItem_Click);
			// 
			// zoomOutToolStripMenuItem
			// 
			this.zoomOutToolStripMenuItem.Name = "zoomOutToolStripMenuItem";
			this.zoomOutToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
			this.zoomOutToolStripMenuItem.Text = "ズームアウト";
			this.zoomOutToolStripMenuItem.Click += new System.EventHandler(this.zoomOutToolStripMenuItem_Click);
			// 
			// viewThisPointToolStripMenuItem
			// 
			this.viewThisPointToolStripMenuItem.Name = "viewThisPointToolStripMenuItem";
			this.viewThisPointToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
			this.viewThisPointToolStripMenuItem.Text = "ここを基準に表示";
			this.viewThisPointToolStripMenuItem.Click += new System.EventHandler(this.viewThisPointToolStripMenuItem_Click);
			// 
			// FileToolStripMenuItem
			// 
			this.FileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.OpenToolStripMenuItem,
            this.ExitToolStripMenuItem});
			this.FileToolStripMenuItem.Name = "FileToolStripMenuItem";
			this.FileToolStripMenuItem.Size = new System.Drawing.Size(70, 20);
			this.FileToolStripMenuItem.Text = "ファイル(&F)";
			// 
			// OpenToolStripMenuItem
			// 
			this.OpenToolStripMenuItem.Name = "OpenToolStripMenuItem";
			this.OpenToolStripMenuItem.Size = new System.Drawing.Size(116, 22);
			this.OpenToolStripMenuItem.Text = "開く(&O)";
			this.OpenToolStripMenuItem.Click += new System.EventHandler(this.OpenToolStripMenuItem_Click);
			// 
			// ExitToolStripMenuItem
			// 
			this.ExitToolStripMenuItem.Name = "ExitToolStripMenuItem";
			this.ExitToolStripMenuItem.Size = new System.Drawing.Size(116, 22);
			this.ExitToolStripMenuItem.Text = "終了(&X)";
			this.ExitToolStripMenuItem.Click += new System.EventHandler(this.ExitToolStripMenuItem_Click);
			// 
			// HelpToolStripMenuItem
			// 
			this.HelpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.AboutToolStripMenuItem});
			this.HelpToolStripMenuItem.Name = "HelpToolStripMenuItem";
			this.HelpToolStripMenuItem.Size = new System.Drawing.Size(67, 20);
			this.HelpToolStripMenuItem.Text = "ヘルプ(&H)";
			// 
			// AboutToolStripMenuItem
			// 
			this.AboutToolStripMenuItem.Name = "AboutToolStripMenuItem";
			this.AboutToolStripMenuItem.Size = new System.Drawing.Size(161, 22);
			this.AboutToolStripMenuItem.Text = "バージョン情報(&A)";
			this.AboutToolStripMenuItem.Click += new System.EventHandler(this.AboutToolStripMenuItem_Click);
			// 
			// mainMenuStrip
			// 
			this.mainMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.FileToolStripMenuItem,
            this.EditToolStripMenuItem,
            this.ViewToolStripMenuItem,
            this.HelpToolStripMenuItem});
			this.mainMenuStrip.Location = new System.Drawing.Point(0, 0);
			this.mainMenuStrip.Name = "mainMenuStrip";
			this.mainMenuStrip.Size = new System.Drawing.Size(1076, 24);
			this.mainMenuStrip.TabIndex = 0;
			this.mainMenuStrip.Text = "menuStrip1";
			// 
			// EditToolStripMenuItem
			// 
			this.EditToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.ReloadToolStripMenuItem});
			this.EditToolStripMenuItem.Name = "EditToolStripMenuItem";
			this.EditToolStripMenuItem.Size = new System.Drawing.Size(60, 20);
			this.EditToolStripMenuItem.Text = "編集(&E)";
			// 
			// ReloadToolStripMenuItem
			// 
			this.ReloadToolStripMenuItem.Name = "ReloadToolStripMenuItem";
			this.ReloadToolStripMenuItem.Size = new System.Drawing.Size(150, 22);
			this.ReloadToolStripMenuItem.Text = "再読み込み(&R)";
			this.ReloadToolStripMenuItem.Click += new System.EventHandler(this.ReloadToolStripMenuItem_Click);
			// 
			// ViewToolStripMenuItem
			// 
			this.ViewToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.StageLaneRootToolStripMenuItem,
            this.OpacityPopupToolStripMenuItem,
            this.DependencyToolStripMenuItem,
            this.ColorSchemeMenuItem,
            this.TransparentToolStripMenuItem,
            this.AdjustXOnScrollToolStripMenuItem});
			this.ViewToolStripMenuItem.Name = "ViewToolStripMenuItem";
			this.ViewToolStripMenuItem.Size = new System.Drawing.Size(61, 20);
			this.ViewToolStripMenuItem.Text = "表示(&V)";
			// 
			// StageLaneRootToolStripMenuItem
			// 
			this.StageLaneRootToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.StageLane1ToolStripMenuItem,
            this.StageLane2ToolStripMenuItem,
            this.StageLane3ToolStripMenuItem,
            this.StageLane4ToolStripMenuItem,
            this.toolStripSeparator1,
            this.RetainHeightToolStripMenuItem});
			this.StageLaneRootToolStripMenuItem.Name = "StageLaneRootToolStripMenuItem";
			this.StageLaneRootToolStripMenuItem.Size = new System.Drawing.Size(215, 22);
			this.StageLaneRootToolStripMenuItem.Text = "レーンごとに表示(&L)";
			// 
			// StageLane1ToolStripMenuItem
			// 
			this.StageLane1ToolStripMenuItem.Name = "StageLane1ToolStripMenuItem";
			this.StageLane1ToolStripMenuItem.Size = new System.Drawing.Size(173, 22);
			this.StageLane1ToolStripMenuItem.Text = "ストール(&S)";
			this.StageLane1ToolStripMenuItem.Click += new System.EventHandler(this.StageLaneToolStripMenuItem_Click);
			// 
			// StageLane2ToolStripMenuItem
			// 
			this.StageLane2ToolStripMenuItem.Name = "StageLane2ToolStripMenuItem";
			this.StageLane2ToolStripMenuItem.Size = new System.Drawing.Size(173, 22);
			this.StageLane2ToolStripMenuItem.Text = "ユーザ0(&0)";
			this.StageLane2ToolStripMenuItem.Click += new System.EventHandler(this.StageLaneToolStripMenuItem_Click);
			// 
			// StageLane3ToolStripMenuItem
			// 
			this.StageLane3ToolStripMenuItem.Name = "StageLane3ToolStripMenuItem";
			this.StageLane3ToolStripMenuItem.Size = new System.Drawing.Size(173, 22);
			this.StageLane3ToolStripMenuItem.Text = "ユーザ1(&1)";
			this.StageLane3ToolStripMenuItem.Click += new System.EventHandler(this.StageLaneToolStripMenuItem_Click);
			// 
			// StageLane4ToolStripMenuItem
			// 
			this.StageLane4ToolStripMenuItem.Name = "StageLane4ToolStripMenuItem";
			this.StageLane4ToolStripMenuItem.Size = new System.Drawing.Size(173, 22);
			this.StageLane4ToolStripMenuItem.Text = "ユーザ2(&2)";
			this.StageLane4ToolStripMenuItem.Click += new System.EventHandler(this.StageLaneToolStripMenuItem_Click);
			// 
			// toolStripSeparator1
			// 
			this.toolStripSeparator1.Name = "toolStripSeparator1";
			this.toolStripSeparator1.Size = new System.Drawing.Size(170, 6);
			// 
			// RetainHeightToolStripMenuItem
			// 
			this.RetainHeightToolStripMenuItem.Name = "RetainHeightToolStripMenuItem";
			this.RetainHeightToolStripMenuItem.Size = new System.Drawing.Size(173, 22);
			this.RetainHeightToolStripMenuItem.Text = "高さを一定にする(&R)";
			this.RetainHeightToolStripMenuItem.Click += new System.EventHandler(this.RetainHeightToolStripMenuItem_Click);
			// 
			// OpacityPopupToolStripMenuItem
			// 
			this.OpacityPopupToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.OpacityStripMenuItem1,
            this.OpacityToolStripMenuItem2,
            this.OpacityToolStripMenuItem3});
			this.OpacityPopupToolStripMenuItem.Name = "OpacityPopupToolStripMenuItem";
			this.OpacityPopupToolStripMenuItem.Size = new System.Drawing.Size(215, 22);
			this.OpacityPopupToolStripMenuItem.Text = "Opacity(&O)";
			// 
			// OpacityStripMenuItem1
			// 
			this.OpacityStripMenuItem1.Name = "OpacityStripMenuItem1";
			this.OpacityStripMenuItem1.Size = new System.Drawing.Size(107, 22);
			this.OpacityStripMenuItem1.Tag = "1.0";
			this.OpacityStripMenuItem1.Text = "100%";
			this.OpacityStripMenuItem1.Click += new System.EventHandler(this.OpacityToolStripMenuItem_Click);
			// 
			// OpacityToolStripMenuItem2
			// 
			this.OpacityToolStripMenuItem2.Name = "OpacityToolStripMenuItem2";
			this.OpacityToolStripMenuItem2.Size = new System.Drawing.Size(107, 22);
			this.OpacityToolStripMenuItem2.Tag = "0.75";
			this.OpacityToolStripMenuItem2.Text = "75%";
			this.OpacityToolStripMenuItem2.Click += new System.EventHandler(this.OpacityToolStripMenuItem_Click);
			// 
			// OpacityToolStripMenuItem3
			// 
			this.OpacityToolStripMenuItem3.Name = "OpacityToolStripMenuItem3";
			this.OpacityToolStripMenuItem3.Size = new System.Drawing.Size(107, 22);
			this.OpacityToolStripMenuItem3.Tag = "0.5";
			this.OpacityToolStripMenuItem3.Text = "50%";
			this.OpacityToolStripMenuItem3.Click += new System.EventHandler(this.OpacityToolStripMenuItem_Click);
			// 
			// DependencyToolStripMenuItem
			// 
			this.DependencyToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.DrawDependencyLeftCurveToolStripMenuItem,
            this.DrawDependencyInsideCurveToolStripMenuItem,
            this.DrawDependencyInsideLineToolStripMenuItem});
			this.DependencyToolStripMenuItem.Name = "DependencyToolStripMenuItem";
			this.DependencyToolStripMenuItem.Size = new System.Drawing.Size(215, 22);
			this.DependencyToolStripMenuItem.Text = "依存関係(&D)";
			// 
			// DrawDependencyLeftCurveToolStripMenuItem
			// 
			this.DrawDependencyLeftCurveToolStripMenuItem.Name = "DrawDependencyLeftCurveToolStripMenuItem";
			this.DrawDependencyLeftCurveToolStripMenuItem.Size = new System.Drawing.Size(175, 22);
			this.DrawDependencyLeftCurveToolStripMenuItem.Text = "左側(曲線)(&I)";
			this.DrawDependencyLeftCurveToolStripMenuItem.Click += new System.EventHandler(this.DrawDependencyLeftCurveToolStripMenuItem1_Click);
			// 
			// DrawDependencyInsideCurveToolStripMenuItem
			// 
			this.DrawDependencyInsideCurveToolStripMenuItem.Name = "DrawDependencyInsideCurveToolStripMenuItem";
			this.DrawDependencyInsideCurveToolStripMenuItem.Size = new System.Drawing.Size(175, 22);
			this.DrawDependencyInsideCurveToolStripMenuItem.Text = "ステージ中(曲線)(&F)";
			this.DrawDependencyInsideCurveToolStripMenuItem.Click += new System.EventHandler(this.DrawDependencyInsideCurveToolStripMenuItem_Click);
			// 
			// DrawDependencyInsideLineToolStripMenuItem
			// 
			this.DrawDependencyInsideLineToolStripMenuItem.Name = "DrawDependencyInsideLineToolStripMenuItem";
			this.DrawDependencyInsideLineToolStripMenuItem.Size = new System.Drawing.Size(175, 22);
			this.DrawDependencyInsideLineToolStripMenuItem.Text = "ステージ中(直線)(&L)";
			this.DrawDependencyInsideLineToolStripMenuItem.Click += new System.EventHandler(this.DrawDependencyInsideLineToolStripMenuItem_Click);
			// 
			// ColorSchemeMenuItem
			// 
			this.ColorSchemeMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.CSDefaultMenuItem,
            this.CSRedMenuItem,
            this.CSBlueMenuItem});
			this.ColorSchemeMenuItem.Name = "ColorSchemeMenuItem";
			this.ColorSchemeMenuItem.Size = new System.Drawing.Size(215, 22);
			this.ColorSchemeMenuItem.Text = "カラースキーム(&C)";
			// 
			// CSDefaultMenuItem
			// 
			this.CSDefaultMenuItem.Name = "CSDefaultMenuItem";
			this.CSDefaultMenuItem.Size = new System.Drawing.Size(137, 22);
			this.CSDefaultMenuItem.Text = "デフォルト(&D)";
			this.CSDefaultMenuItem.Click += new System.EventHandler(this.CSMenuItem_Click);
			// 
			// CSRedMenuItem
			// 
			this.CSRedMenuItem.Name = "CSRedMenuItem";
			this.CSRedMenuItem.Size = new System.Drawing.Size(137, 22);
			this.CSRedMenuItem.Text = "赤(&R)";
			this.CSRedMenuItem.Click += new System.EventHandler(this.CSMenuItem_Click);
			// 
			// CSBlueMenuItem
			// 
			this.CSBlueMenuItem.Name = "CSBlueMenuItem";
			this.CSBlueMenuItem.Size = new System.Drawing.Size(137, 22);
			this.CSBlueMenuItem.Text = "青(&B)";
			this.CSBlueMenuItem.Click += new System.EventHandler(this.CSMenuItem_Click);
			// 
			// TransparentToolStripMenuItem
			// 
			this.TransparentToolStripMenuItem.Name = "TransparentToolStripMenuItem";
			this.TransparentToolStripMenuItem.Size = new System.Drawing.Size(215, 22);
			this.TransparentToolStripMenuItem.Text = "背景の透明化(&T)";
			this.TransparentToolStripMenuItem.Click += new System.EventHandler(this.TransparentToolStripMenuItem_Click);
			// 
			// AdjustXOnScrollToolStripMenuItem
			// 
			this.AdjustXOnScrollToolStripMenuItem.Name = "AdjustXOnScrollToolStripMenuItem";
			this.AdjustXOnScrollToolStripMenuItem.Size = new System.Drawing.Size(215, 22);
			this.AdjustXOnScrollToolStripMenuItem.Text = "スクロール時X座標を調節(&X)";
			this.AdjustXOnScrollToolStripMenuItem.Click += new System.EventHandler(this.AdjustXOnScrollToolStripMenuItem_Click);
			// 
			// trackBarZoom
			// 
			this.trackBarZoom.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.trackBarZoom.LargeChange = 1;
			this.trackBarZoom.Location = new System.Drawing.Point(1046, 27);
			this.trackBarZoom.Maximum = 3;
			this.trackBarZoom.Minimum = -11;
			this.trackBarZoom.Name = "trackBarZoom";
			this.trackBarZoom.Orientation = System.Windows.Forms.Orientation.Vertical;
			this.trackBarZoom.Size = new System.Drawing.Size(45, 667);
			this.trackBarZoom.TabIndex = 4;
			this.trackBarZoom.Value = 3;
			this.trackBarZoom.ValueChanged += new System.EventHandler(this.trackBarZoom_ValueChanged);
			this.trackBarZoom.MouseMove += new System.Windows.Forms.MouseEventHandler(this.trackBarZoom_MouseMove);
			this.trackBarZoom.MouseUp += new System.Windows.Forms.MouseEventHandler(this.trackBarZoom_MouseUp);
			// 
			// panelMainView
			// 
			this.panelMainView.AutoScroll = true;
			this.panelMainView.BackColor = System.Drawing.SystemColors.Window;
			this.panelMainView.ContextMenuStrip = this.contextMenuStrip;
			this.panelMainView.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panelMainView.Location = new System.Drawing.Point(0, 0);
			this.panelMainView.Name = "panelMainView";
			this.panelMainView.ScrollPosition = new System.Drawing.Point(0, 0);
			this.panelMainView.Size = new System.Drawing.Size(724, 667);
			this.panelMainView.TabIndex = 3;
			this.panelMainView.VirtualSize = new System.Drawing.Size(0, 0);
			this.panelMainView.Scroll += new System.Windows.Forms.ScrollEventHandler(this.panelMainView_Scroll);
			this.panelMainView.Click += new System.EventHandler(this.panelMainView_Click);
			this.panelMainView.Paint += new System.Windows.Forms.PaintEventHandler(this.panelMainView_Paint);
			this.panelMainView.DoubleClick += new System.EventHandler(this.panelMainView_DoubleClick);
			this.panelMainView.MouseDown += new System.Windows.Forms.MouseEventHandler(this.panelMainView_MouseDown);
			this.panelMainView.MouseMove += new System.Windows.Forms.MouseEventHandler(this.panelMainView_MouseMove);
			this.panelMainView.MouseUp += new System.Windows.Forms.MouseEventHandler(this.panelMainView_MouseUp);
			// 
			// MainForm
			// 
			this.AllowDrop = true;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(1076, 694);
			this.Controls.Add(this.splitContainer1);
			this.Controls.Add(this.trackBarZoom);
			this.Controls.Add(this.mainMenuStrip);
			this.MainMenuStrip = this.mainMenuStrip;
			this.Name = "MainForm";
			this.Text = "Kanata";
			this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.MainForm_FormClosed);
			this.DragDrop += new System.Windows.Forms.DragEventHandler(this.MainForm_DragDrop);
			this.DragEnter += new System.Windows.Forms.DragEventHandler(this.MainForm_DragEnter);
			this.splitContainer1.Panel1.ResumeLayout(false);
			this.splitContainer1.Panel2.ResumeLayout(false);
			this.splitContainer1.ResumeLayout(false);
			this.contextMenuStrip.ResumeLayout(false);
			this.mainMenuStrip.ResumeLayout(false);
			this.mainMenuStrip.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.trackBarZoom)).EndInit();
			this.ResumeLayout(false);
			this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.SplitContainer splitContainer1;
		private System.Windows.Forms.Panel panelLabel;
        private System.Windows.Forms.ToolTip toolTip;
        private System.Windows.Forms.ToolStripMenuItem FileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem OpenToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem ExitToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem HelpToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem AboutToolStripMenuItem;
		private System.Windows.Forms.MenuStrip mainMenuStrip;
		private System.Windows.Forms.TrackBar trackBarZoom;
        private System.Windows.Forms.ToolStripMenuItem ViewToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem OpacityPopupToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem OpacityStripMenuItem1;
        private System.Windows.Forms.ToolStripMenuItem OpacityToolStripMenuItem2;
        private System.Windows.Forms.ToolStripMenuItem OpacityToolStripMenuItem3;
        private System.Windows.Forms.ToolStripMenuItem DependencyToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem DrawDependencyLeftCurveToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem DrawDependencyInsideCurveToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem TransparentToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem AdjustXOnScrollToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem EditToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem ReloadToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem ColorSchemeMenuItem;
        private System.Windows.Forms.ToolStripMenuItem CSDefaultMenuItem;
        private System.Windows.Forms.ToolStripMenuItem CSRedMenuItem;
        private System.Windows.Forms.ToolStripMenuItem CSBlueMenuItem;
		private System.Windows.Forms.ContextMenuStrip contextMenuStrip;
		private System.Windows.Forms.ToolStripMenuItem zoomInToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem viewThisPointToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem zoomOutToolStripMenuItem;
		private LargePanel panelMainView;
		private System.Windows.Forms.ToolStripMenuItem StageLaneRootToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem StageLane1ToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem StageLane2ToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem StageLane3ToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem StageLane4ToolStripMenuItem;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
		private System.Windows.Forms.ToolStripMenuItem RetainHeightToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem DrawDependencyInsideLineToolStripMenuItem;
    }
}

