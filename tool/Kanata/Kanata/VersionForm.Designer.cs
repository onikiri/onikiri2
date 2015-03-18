namespace Kanata
{
    partial class VersionForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
			this.OKButton = new System.Windows.Forms.Button();
			this.KanataName = new System.Windows.Forms.Label();
			this.Ichibayashi = new System.Windows.Forms.Label();
			this.Shioya = new System.Windows.Forms.Label();
			this.Version = new System.Windows.Forms.Label();
			this.Horio = new System.Windows.Forms.Label();
			this.SuspendLayout();
			// 
			// OKButton
			// 
			this.OKButton.Location = new System.Drawing.Point(200, 128);
			this.OKButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
			this.OKButton.Name = "OKButton";
			this.OKButton.Size = new System.Drawing.Size(87, 26);
			this.OKButton.TabIndex = 0;
			this.OKButton.Text = "OK";
			this.OKButton.UseVisualStyleBackColor = true;
			this.OKButton.Click += new System.EventHandler(this.OKButton_Click);
			// 
			// KanataName
			// 
			this.KanataName.AutoSize = true;
			this.KanataName.Location = new System.Drawing.Point(16, 16);
			this.KanataName.Name = "KanataName";
			this.KanataName.Size = new System.Drawing.Size(156, 18);
			this.KanataName.TabIndex = 1;
			this.KanataName.Text = "Kanata Pipeline Visualizer";
			// 
			// Ichibayashi
			// 
			this.Ichibayashi.AutoSize = true;
			this.Ichibayashi.Location = new System.Drawing.Point(16, 64);
			this.Ichibayashi.Name = "Ichibayashi";
			this.Ichibayashi.Size = new System.Drawing.Size(270, 18);
			this.Ichibayashi.TabIndex = 2;
			this.Ichibayashi.Text = "Copyright (C) 2006-2008 Hironori Ichibayashi";
			// 
			// Shioya
			// 
			this.Shioya.AutoSize = true;
			this.Shioya.Location = new System.Drawing.Point(16, 100);
			this.Shioya.Name = "Shioya";
			this.Shioya.Size = new System.Drawing.Size(232, 18);
			this.Shioya.TabIndex = 3;
			this.Shioya.Text = "Copyright (C) 2008-2013 Ryota Shioya";
			// 
			// Version
			// 
			this.Version.AutoSize = true;
			this.Version.Location = new System.Drawing.Point(16, 34);
			this.Version.Name = "Version";
			this.Version.Size = new System.Drawing.Size(79, 18);
			this.Version.TabIndex = 4;
			this.Version.Text = "Version 1.24";
			// 
			// Horio
			// 
			this.Horio.AutoSize = true;
			this.Horio.Location = new System.Drawing.Point(16, 82);
			this.Horio.Name = "Horio";
			this.Horio.Size = new System.Drawing.Size(225, 18);
			this.Horio.TabIndex = 5;
			this.Horio.Text = "Copyright (C) 2008-2009 Kazuo Horio";
			// 
			// VersionForm
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 18F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(302, 169);
			this.Controls.Add(this.Horio);
			this.Controls.Add(this.Version);
			this.Controls.Add(this.Shioya);
			this.Controls.Add(this.Ichibayashi);
			this.Controls.Add(this.KanataName);
			this.Controls.Add(this.OKButton);
			this.Font = new System.Drawing.Font("メイリオ", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(128)));
			this.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
			this.Name = "VersionForm";
			this.Text = "バージョン情報";
			this.ResumeLayout(false);
			this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button OKButton;
        private System.Windows.Forms.Label KanataName;
        private System.Windows.Forms.Label Ichibayashi;
        private System.Windows.Forms.Label Shioya;
        private System.Windows.Forms.Label Version;
        private System.Windows.Forms.Label Horio;
    }
}