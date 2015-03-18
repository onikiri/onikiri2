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
	public partial class MainForm
	{
		[Serializable]
		public class ViewSetting
		{
			public int ZoomFactor;

			// zoomFactor >= 0のとき
			public int CellMarginHeight;

			public bool DrawOutline = false;
			public bool DrawStageName = false;
			public bool DrawInsnName = false;
			public SizeF CellSize;

			public bool DrawDependencyLeftCurve = false;	// Producerの左端からConsumerの左端への矢印
			public bool DrawDependencyInsideCurve = false;	// ProducerのXからConsumerのXへの矢印（曲線）
			public bool DrawDependencyInsideLine = false;	// ProducerのXからConsumerのXへの矢印（直線）
			public float DependencyArrowheadLength;			// 矢印の先端の長さ (三角形の高さ)
			public float DependencyArrowWidth;
		}

		[Serializable]
		public class Configuration
		{
			public bool DrawDependencyLeftCurve = false;
			public bool DrawDependencyInsideCurve = false;
			public bool DrawDependencyInsideLine = false;
			public bool AdjustXOnScroll = true;
			public bool MainViewTransparent = false;
			public uint MainViewBackColorARGB = 0xffffffff;
			public int  ColorScheme = MainForm.ColorScheme.Default;
			public int MaxLane = 10;
			public List<bool> DrawInSeparateLane = new List<bool>();
			public bool RetainOriginalGridSize = true; 

			public void Default()
			{
				DrawInSeparateLane.Clear();
				for( int i = 0; i < MaxLane; i++ ) {
					DrawInSeparateLane.Add( false );
				}
				DrawInSeparateLane[ 0 ] = true;
				DrawInSeparateLane[ 1 ] = false;
			}
		}

		// 表示設定 (暫定)
		private List<ViewSetting> viewSettings = new List<ViewSetting>();
		private ViewSetting currentViewSetting = null;

		private Configuration config = new Configuration();

		public class StageColorDef
		{
			public string Name { get; set; }
			public string Color { get; set; }
		}

		private class ColorScheme
		{
			public static int Default { get { return 0; } }
			public static int Red { get { return 1; } }
			public static int Blue { get { return 2; } }
		}

		private string ConfigurationFileName
		{
			get { return "config.xml"; }
		}

		private bool DrawDependencyLeftCurve
		{
			get { return config.DrawDependencyLeftCurve; }
			set { config.DrawDependencyLeftCurve = value; }
		}
		private bool DrawDependencyInsideCurve
		{
			get { return config.DrawDependencyInsideCurve; }
			set { config.DrawDependencyInsideCurve = value; }
		}
		private bool DrawDependencyInsideLine
		{
			get { return config.DrawDependencyInsideLine; }
			set { config.DrawDependencyInsideLine = value; }
		}
		private bool AdjustXOnScroll
		{
			get { return config.AdjustXOnScroll; }
			set { config.AdjustXOnScroll = value; }
		}

		private int ZoomFactor
		{
			get { return currentViewSetting.ZoomFactor; }
			set
			{
				if( value < 0 ) {
					ViewSetting vs = new ViewSetting();
					ViewSetting vs0 = viewSettings[ 0 ];

					vs.DrawInsnName = vs.DrawOutline = vs.DrawStageName = false;
					vs.CellMarginHeight = 0;
					vs.ZoomFactor = value;
					vs.CellSize = new SizeF( 
						vs0.CellSize.Width * (float)Math.Pow( 2.0, value ), 
						vs0.CellSize.Height * (float)Math.Pow( 2.0, value ) 
					);

					ApplyViewSetting( vs );
				}
				else {
					ApplyViewSetting( viewSettings[ value ] );
				}
			}
		}
		
		private int CellMarginHeight
		{
			get { return currentViewSetting.CellMarginHeight; }
		}

		// ステージ毎のカラースキームを読み込む
		private void ReadColorDef()
		{
			StreamReader ins = null;
			try {
				XmlSerializer serializer = 
					new XmlSerializer( typeof( List<List<StageColorDef>> ) );

				ins = new StreamReader( "colors.xml" );
				List<List<StageColorDef>> colorDefs =
					(List<List<StageColorDef>>)serializer.Deserialize( ins );

				brushMap.Clear();
				foreach( var segment in colorDefs ) {
					Dictionary<String, StageBrushSet> map = 
						new Dictionary<String, StageBrushSet>();
					brushMap.Add( map );

					foreach( var def in segment ) {
						int colorNum = 
							int.Parse( def.Color, System.Globalization.NumberStyles.HexNumber );

						map[ def.Name ] =
							new StageBrushSet(
								GetStageBrush( colorNum, false ),
								GetStageBrush( colorNum, true )
							);
					}
				}

			}
			catch( Exception e ) {
				MessageBox.Show( "Error: " + e.Message );
			}
			finally {
				if( ins != null )
					ins.Close();
				for( int i = brushMap.Count; i < loginfo.StageNames.Count; i++ ) {
					Dictionary<String, StageBrushSet> map = 
						new Dictionary<String, StageBrushSet>();
					brushMap.Add( map );
				}
			}
		}

		private void MakeDefaultViewSettings()
		{
			ViewSetting vs;

			// 0
			vs = new ViewSetting();
			vs.ZoomFactor = 0;
			vs.CellMarginHeight = 0;
			vs.DrawOutline = false;
			vs.DrawInsnName = false;
			vs.DrawStageName = false;
			vs.DrawDependencyLeftCurve = false;
			vs.DrawDependencyInsideCurve = false;
			vs.DrawDependencyInsideLine = false;
			vs.CellSize = new SizeF( 1.0f, 1.0f );
			vs.DependencyArrowheadLength = 1.0f;
			vs.DependencyArrowWidth = 1.0f;
			viewSettings.Add( vs );

			// 1
			vs = new ViewSetting();
			vs.ZoomFactor = 1;
			vs.CellMarginHeight = 0;
			vs.DrawOutline = false;
			vs.DrawInsnName = false;
			vs.DrawStageName = false;
			vs.DrawDependencyLeftCurve = true;
			vs.DrawDependencyInsideCurve = true;
			vs.DrawDependencyInsideLine = true;
			vs.CellSize = new SizeF( 8.0f, 6.0f );
			vs.DependencyArrowheadLength = vs.CellSize.Width / 5.0f;
			vs.DependencyArrowWidth = 1.0f;
			viewSettings.Add( vs );

			// 2
			vs = new ViewSetting();
			vs.ZoomFactor = 2;
			vs.CellMarginHeight = 1;
			vs.DrawOutline = true;
			vs.DrawInsnName = false;
			vs.DrawStageName = false;
			vs.DrawDependencyLeftCurve = true;
			vs.DrawDependencyInsideCurve = true;
			vs.DrawDependencyInsideLine = true;
			vs.CellSize = new SizeF( 16.0f, 12.0f );
			vs.DependencyArrowheadLength = vs.CellSize.Width / 5.0f;
			vs.DependencyArrowWidth = 1.0f;
			viewSettings.Add( vs );

			// 3
			vs = new ViewSetting();
			vs.ZoomFactor = 3;
			vs.CellMarginHeight = 2;
			vs.DrawOutline = true;
			vs.DrawInsnName = true;
			vs.DrawStageName = true;
			vs.DrawDependencyLeftCurve = true;
			vs.DrawDependencyInsideCurve = true;
			vs.DrawDependencyInsideLine = true;
			vs.CellSize = new SizeF( 32.0f, 24.0f );
			vs.DependencyArrowheadLength = vs.CellSize.Width / 5.0f;
			vs.DependencyArrowWidth = 2.0f;
			viewSettings.Add( vs );

			currentViewSetting = vs;

			trackBarZoom.Maximum = viewSettings.Count - 1;

			WriteViewSettings();
		}

		private void WriteViewSettings()
		{
			XmlSerializer mySerializer = new XmlSerializer( typeof( List<ViewSetting> ) );
			StreamWriter myWriter = new StreamWriter( "viewsettings.xml" );
			mySerializer.Serialize( myWriter, viewSettings );
			myWriter.Close();
		}

		private void ReadViewSettings()
		{
			XmlSerializer mySerializer = new XmlSerializer( typeof( List<ViewSetting> ) );
			FileStream myFileStream = new FileStream( "viewsettings.xml", FileMode.Open );
			viewSettings = (List<ViewSetting>)mySerializer.Deserialize( myFileStream );

			currentViewSetting = viewSettings[ viewSettings.Count - 1 ];
			trackBarZoom.Maximum = viewSettings.Count - 1;
			myFileStream.Close();
		}

		private void WriteConfiguration()
		{
			XmlSerializer mySerializer = new XmlSerializer( typeof( Configuration ) );
			StreamWriter myWriter = new StreamWriter( ConfigurationFileName );
			mySerializer.Serialize( myWriter, config );
			myWriter.Close();
		}

		private void ReadConfiguration()
		{
			XmlSerializer mySerializer = new XmlSerializer( typeof( Configuration ) );
			FileStream myFileStream = new FileStream( ConfigurationFileName, FileMode.Open );
			config = (Configuration)mySerializer.Deserialize( myFileStream );
			myFileStream.Close();
		}

		private void InitializeConfiguration()
		{
			config.MainViewBackColorARGB = (uint)panelMainView.BackColor.ToArgb();

			try {
				ReadConfiguration();
			}
			catch( FileNotFoundException ) {
				config = new Configuration();
				config.Default();
			}

			try {
				ReadViewSettings();
			}
			catch( FileNotFoundException ) {
				MakeDefaultViewSettings();
			}
		}
	}
}

