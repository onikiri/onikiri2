using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Kanata
{
	public partial class LoadingForm : Form
	{
		LogLoader m_loader;
		string m_fileName;

		public LoadingForm( LogLoader loader, string fileName )
		{
			m_loader = loader;
			m_fileName = fileName;
			InitializeComponent();
		}
		protected override void OnClosing( CancelEventArgs e )
        {
            base.OnClosing(e);
        }
        private void ExitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        public delegate void AppendResultCallback(string text);
		private void AppendResult( string text )
		{
			if( textResult.InvokeRequired ) {
				AppendResultCallback d = new AppendResultCallback( AppendResult );
				Invoke( d, new object[] { text } );
			}
			else {
				textResult.AppendText( text );
			}
		}

		private void LoadingFrom_Shown( object sender, EventArgs e )
        {
            AppendResult(string.Format("{0} ver.{1}\r\n", Application.ProductName, Application.ProductVersion));
			AppendResult( string.Format( "サポートする鬼斬のログ : {0:X4}\r\n", m_loader.OnikiriKanataFileVersion ) );
			AppendResult("\r\n");
            AppendResult("処理するログファイルをドロップしてください。\r\n");
            AppendResult("\r\n");
            RunWorker();
        }


        private void RunWorker()
        {
            try {
                worker.RunWorkerAsync( m_fileName );
				labelFile.Text = m_fileName;
                buttonCancel.Enabled = true;
            }
            catch (InvalidOperationException) {
                // 既に実行中なのでOK
            }
        }

        private void worker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            progressBar.Value = e.ProgressPercentage;
        }

        // This event handler deals with the results of the
        // background operation.
        private void worker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            if (e.Error != null)
            {
                AppendResult(string.Format("エラー({0})\r\n", e.Error.Message));
                MessageBox.Show(e.Error.Message, "エラー", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
            else if (e.Cancelled)
            {
                AppendResult("キャンセル\r\n");
            }
            else {
				if( m_loader.SuccessfullyLoaded ) {
					AppendResult( "完了\r\n" );
					this.Close();
				}
			}

            buttonCancel.Enabled = false;

        }

        private void worker_DoWork(object sender, DoWorkEventArgs ea)
        {
            BackgroundWorker worker = sender as BackgroundWorker;

			string fileName = (string)ea.Argument;

			AppendResult( string.Format( "Loading {0} ... ", fileName ) );

			System.Diagnostics.Stopwatch sw = new System.Diagnostics.Stopwatch();
			sw.Start();

            ea.Result = false;

            try {
				m_loader.Load( fileName, worker );

				List<String> errors = m_loader.Errors;
				if( errors.Count > 0 ) {
					String errorMsg = "";
					for( int i = 0; i < 10 && i < errors.Count; i++ ) {
						errorMsg += string.Format( "{0}\n\r", errors[ i ] );
					}
					throw new ApplicationException( errorMsg );
				}
				

				ea.Result = true;

				sw.Stop();
				string msg = string.Format( "\r\nElapsed {0} sec.\r\n", sw.Elapsed );
				AppendResult( msg );
			}
            catch (Exception e) {
				string msg = string.Format( "\r\n{0}({1}): {2}\r\n", fileName, m_loader.CurrentLine, e.Message );
				AppendResult( msg );
                MessageBox.Show( msg, "エラー", MessageBoxButtons.OK, MessageBoxIcon.Exclamation );
                ea.Result = false;
            }
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (worker.IsBusy) {
                MessageBox.Show("変換中は終了できません", "エラー", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                e.Cancel = true;
            }
        }

        private void buttonCancel_Click(object sender, EventArgs e)
        {
            if (worker.IsBusy) {
                worker.CancelAsync();
            }
        }


	}
}
