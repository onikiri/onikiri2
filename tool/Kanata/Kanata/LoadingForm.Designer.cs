namespace Kanata
{
	partial class LoadingForm
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose( bool disposing )
		{
			if( disposing && ( components != null ) ) {
				components.Dispose();
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.textResult = new System.Windows.Forms.TextBox();
			this.progressBar = new System.Windows.Forms.ProgressBar();
			this.labelFile = new System.Windows.Forms.Label();
			this.buttonCancel = new System.Windows.Forms.Button();
			this.worker = new System.ComponentModel.BackgroundWorker();
			this.SuspendLayout();
			// 
			// textResult
			// 
			this.textResult.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.textResult.BackColor = System.Drawing.SystemColors.Window;
			this.textResult.Location = new System.Drawing.Point(12, 55);
			this.textResult.Multiline = true;
			this.textResult.Name = "textResult";
			this.textResult.ReadOnly = true;
			this.textResult.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
			this.textResult.Size = new System.Drawing.Size(506, 301);
			this.textResult.TabIndex = 1;
			// 
			// progressBar
			// 
			this.progressBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.progressBar.Location = new System.Drawing.Point(12, 26);
			this.progressBar.Maximum = 1000;
			this.progressBar.Name = "progressBar";
			this.progressBar.Size = new System.Drawing.Size(506, 23);
			this.progressBar.Style = System.Windows.Forms.ProgressBarStyle.Continuous;
			this.progressBar.TabIndex = 4;
			// 
			// labelFile
			// 
			this.labelFile.AutoSize = true;
			this.labelFile.Location = new System.Drawing.Point(12, 9);
			this.labelFile.Name = "labelFile";
			this.labelFile.Padding = new System.Windows.Forms.Padding(0, 0, 0, 2);
			this.labelFile.Size = new System.Drawing.Size(48, 14);
			this.labelFile.TabIndex = 3;
			this.labelFile.Text = "filename";
			// 
			// buttonCancel
			// 
			this.buttonCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonCancel.Enabled = false;
			this.buttonCancel.Location = new System.Drawing.Point(374, 362);
			this.buttonCancel.Name = "buttonCancel";
			this.buttonCancel.Size = new System.Drawing.Size(144, 32);
			this.buttonCancel.TabIndex = 5;
			this.buttonCancel.Text = "キャンセル";
			this.buttonCancel.UseVisualStyleBackColor = true;
			this.buttonCancel.Click += new System.EventHandler(this.buttonCancel_Click);
			// 
			// worker
			// 
			this.worker.WorkerReportsProgress = true;
			this.worker.WorkerSupportsCancellation = true;
			this.worker.DoWork += new System.ComponentModel.DoWorkEventHandler(this.worker_DoWork);
			this.worker.ProgressChanged += new System.ComponentModel.ProgressChangedEventHandler(this.worker_ProgressChanged);
			this.worker.RunWorkerCompleted += new System.ComponentModel.RunWorkerCompletedEventHandler(this.worker_RunWorkerCompleted);
			// 
			// LoadingForm
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(530, 406);
			this.Controls.Add(this.buttonCancel);
			this.Controls.Add(this.progressBar);
			this.Controls.Add(this.labelFile);
			this.Controls.Add(this.textResult);
			this.Name = "LoadingForm";
			this.Text = "LoadingForm";
			this.Shown += new System.EventHandler(this.LoadingFrom_Shown);
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.TextBox textResult;
		private System.Windows.Forms.ProgressBar progressBar;
		private System.Windows.Forms.Label labelFile;
		private System.Windows.Forms.Button buttonCancel;
		private System.ComponentModel.BackgroundWorker worker;
	}
}