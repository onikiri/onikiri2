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
		// 文字列(stage名)と対応する色のbrush
		private class StageBrushSet
		{
			public Brush normal;
			public Brush flushed;

			public StageBrushSet( Brush normal, Brush flushed )
			{
				this.normal = normal;
				this.flushed = flushed;
			}
		}

		private List< List<StageBrushSet> > stageBrushes;
		private List<Dictionary<String, StageBrushSet> >
			brushMap = new List<Dictionary<string, StageBrushSet>>();

		// バックグラウンド用のブラシセット
		private List<Brush> backGroundBrushes = new List<Brush>();
		private Brush invalidBackGroundBrush = new SolidBrush( Color.DarkGray );

		private StageBrushSet DefaultStageBrush
		{
			get
			{
				return new StageBrushSet(
					GetStageBrush( Color.PaleGreen.ToArgb(), false ),
					GetStageBrush( Color.PaleGreen.ToArgb(), true )
				);
			}
		}

		// 表示フォント
		private Font stageFont;
		private Font labelFont;
		private Font LabelFont
		{
			get { return labelFont; }
		}
		private Font StageFont
		{
			get { return stageFont; }
		}

		private String DefaultFontName()
		{
			return "Verdena";
		}

		// 矢印の設定は未実装
		// シリアライズ不能な型のプロパティ (not 変数) がクラスに存在するとシリアライズできない？
		private class DependencyArrow
		{
			public Brush brush;
			public Pen   pen;

			public DependencyArrow( Color color, float width )
			{
				brush = new SolidBrush( color );
				pen = new Pen( color, width );
			}
		};
		private List<DependencyArrow> dependencyArrows = new List<DependencyArrow>();


		// DrawStage 用のオフスクリーンビットマップ
		Bitmap drawStageBMP = new Bitmap( 16, 16 );
	
		private Color AddColor( Color color, int r, int g, int b )
		{
			r += color.R; if( r < 0 ) r = 0; if( r > 255 ) r = 255;
			g += color.G; if( g < 0 ) g = 0; if( g > 255 ) g = 255;
			b += color.B; if( b < 0 ) b = 0; if( b > 255 ) b = 255;
			return Color.FromArgb( color.A, r, g, b );
		}

		// 基準色からグラデーションのかかったブラシを作成
		private Brush GetStageBrush( int colorNum, bool darker )
		{
			float cellHeight = currentViewSetting.CellSize.Height;

			Color baseColor = Color.FromArgb( colorNum );
			Color topColor = AddColor( baseColor, 50, 50, 50 );
			Color midColor = AddColor( baseColor, 20, 20, 20 );
			Color bottomColor = AddColor( baseColor, 0, 0, 0 );

			if( darker ) {
				int sub = -80;
				topColor = AddColor( topColor, sub, sub, sub );
				midColor = AddColor( midColor, sub, sub, sub );
				bottomColor = AddColor( bottomColor, sub, sub, sub );
			}

			ColorBlend cb = new ColorBlend( 3 );
			cb.Positions = new float[] { 0.0f, 0.65f, 1.0f };
			cb.Colors = new Color[] { topColor, midColor, bottomColor };

			LinearGradientBrush lb =
                new LinearGradientBrush(
					new PointF( 0.0f, 0.0f ),
					new PointF( 0.0f, cellHeight ),
					topColor,
					bottomColor
				);
			lb.InterpolationColors = cb;

			return lb;
		}

		// ステージ表示用のブラシを作成
		private void MakeStageBrushes()
		{
			if( loginfo.StageNames == null )
				return;

			ReadColorDef();

			StageBrushSet blueBrush = 
				new StageBrushSet(
				GetStageBrush( Color.MediumTurquoise.ToArgb(), false ),
				GetStageBrush( Color.MediumTurquoise.ToArgb(), true )
			);
			StageBrushSet redBrush = 
				new StageBrushSet(
				GetStageBrush( Color.Salmon.ToArgb(), false ),
				GetStageBrush( Color.Salmon.ToArgb(), true )
			);

			stageBrushes = new List<List<StageBrushSet>>();
			for( int segmentID = 0; segmentID < loginfo.StageNames.Count; segmentID++ ) {
				var stageNames = loginfo.StageNames[ segmentID ];

				List<StageBrushSet> brushes = new List<StageBrushSet>();
				foreach( var name in stageNames ) {

					StageBrushSet set = DefaultStageBrush;

					if( config.ColorScheme == ColorScheme.Default ) {
						if( brushMap[ segmentID ].ContainsKey( name ) )
							set = brushMap[ segmentID ][ name ];
					}
					else if( config.ColorScheme == ColorScheme.Blue ) {
						set = blueBrush;
					}
					else if( config.ColorScheme == ColorScheme.Red ) {
						set = redBrush;
					}

					brushes.Add( set );
				}

				stageBrushes.Add( brushes );
			}

		}

		// 依存関係表示用のブラシ，ペンを作成
		private void MakeDependencyArrow()
		{
			dependencyArrows = new List<DependencyArrow>();
			Color[] colorSet = 
			{
				Color.Crimson,
				Color.Blue,
				Color.Green,
				Color.Yellow,
			};

			foreach( Color color in colorSet ) {
				DependencyArrow arrow = new DependencyArrow( color, 1.0f );
				dependencyArrows.Add( arrow );
			}
		}

		// 表示フォントの作成
		private Font GetDefaultFont()
		{
			Font font;
			font = new Font( DefaultFontName(), SystemFonts.DefaultFont.Size - 1 );
			if( font == null ) {
				font = SystemFonts.DefaultFont;
			}
			return font;
		}

		// フォントの初期化
		private void InitializeFont()
		{
			stageFont = GetDefaultFont();
			labelFont = GetDefaultFont();
		}

		private void InitializeView()
		{
			InitializeFont();
		}

		// ステージ一つ分の描画
		private void DrawStage(
			Graphics renderTarget,
			int stageID,
			int segmentID,
			int x,
			int y,
			int length,
			string stageName,
			bool flushed
		)
		{
			if( length == 0 )
				return;

			SizeF cell = coordinateSystem.Cell;
			Size stageSize = 
				new Size(
					length * (int)cell.Width,
					(int)cell.Height - CellMarginHeight * 2
				);

			Rectangle rect = new Rectangle(
				x,
				y, 
				stageSize.Width, 
				stageSize.Height 
			);


			StageBrushSet brushSet = stageBrushes[ segmentID ][ stageID ];
			Brush brush = flushed ? brushSet.flushed : brushSet.normal;
			renderTarget.FillRectangle( brush, rect );
			if( currentViewSetting.DrawOutline ) {
				renderTarget.DrawRectangle( Pens.Black, rect );
			}

			if( currentViewSetting.DrawStageName ) {
				Font stageFont = StageFont;
				PointF p = rect.Location;
				SizeF size = renderTarget.MeasureString( stageName, stageFont );
				p.X += cell.Width / 2 - size.Width / 2;
				p.Y += cell.Height / 2 - size.Height / 2;
				renderTarget.DrawString( stageName, stageFont, Brushes.Black, p );

				for( int j = 1; j < length; j++ ) {
					p.X = rect.Location.X + j * cell.Width;
					if( !renderTarget.ClipBounds.IntersectsWith( new RectangleF( p, cell ) ) )
						continue;
					String s = ( j + 1 ).ToString();
					size = renderTarget.MeasureString( s, stageFont );
					p.X += cell.Width / 2 - size.Width / 2;
					renderTarget.DrawString( s, stageFont, Brushes.Gray, p );
				}
			}
		}

		//命令一つ分の描画
		private void DrawInsn( Graphics g, Insn insn )
		{
			int bufferWidth  = (int)( insn.EndCycle - insn.StartCycle ) * (int)coordinateSystem.Grid.Width;
			int bufferHeight = (int)coordinateSystem.Grid.Height - CellMarginHeight * 2;
			Rectangle rect = new Rectangle( 0, 0, bufferWidth, bufferHeight );

			bool drawOffscreen = false;

			if( drawOffscreen ) {
				// Graphics::TranslateTransform() がバグっており，2048命令以上の部分でグラデーションがおかしくなるので，
				// オフスクリーンに一度描いたあと転送する
				if( drawStageBMP.Height < bufferHeight + 16 ||
					drawStageBMP.Width < bufferWidth + 16
				) {
					drawStageBMP =
						new Bitmap(
							Math.Max( drawStageBMP.Width, bufferWidth + 16 ),
							Math.Max( drawStageBMP.Height, bufferHeight + 16 ),
							g
						);
				}
			}

			Graphics renderTarget = drawOffscreen ? Graphics.FromImage( drawStageBMP ) : g;
			int baseY = drawOffscreen ? 0 : coordinateSystem.YAtInsnId( insn.Id ) + CellMarginHeight;
			int baseX = drawOffscreen ? 0 : coordinateSystem.XAtCycle( insn.StartCycle );
			int segmentY = 0;

			Font stageFont = StageFont;
			for( int segmentID = 0; segmentID < insn.StageSegments.Count; segmentID++ ) {
				var stages = insn.StageSegments[ segmentID ];
				var nameMap = loginfo.StageNames[ segmentID ];
				int y = baseY;
				if( segmentID != 0 && config.DrawInSeparateLane[ segmentID ] ) {
					segmentY += (int)coordinateSystem.Cell.Height;
					y += segmentY;
				}
				foreach( var stage in stages ) {
					int x = stage.BeginRelCycle * (int)coordinateSystem.Grid.Width + baseX;
					int length = (int)stage.Length;
					string stageName = nameMap[ stage.Id ];
					DrawStage( renderTarget, stage.Id, segmentID, x, y, length, stageName, insn.Flushed );
				}
			}

			if( drawOffscreen ) {
				int dstY = coordinateSystem.YAtInsnId( insn.Id ) + CellMarginHeight;
				int dstX = coordinateSystem.XAtCycle( insn.StartCycle );

				if( currentViewSetting.DrawOutline ) {
					// DrawRectangle has a bug that draws a rectangle that is 1-pixel larger than specified size.
					rect.Inflate( 1, 1 );
				}

				g.DrawImage( drawStageBMP, dstX, dstY, rect, GraphicsUnit.Pixel );
				renderTarget.Dispose();
			}

		}

		private struct Dependency
		{
			public Dependency( ulong prod, ulong cons, int type )
			{
				Producer = prod;
				Consumer = cons;
				Type = type;
			}
			public ulong Producer;
			public ulong Consumer;
			public int Type;
		}

		private void DrawDependency( Graphics g, ulong drawIdFrom, ulong drawIdTo, ulong viewIdFrom, ulong viewIdTo )
		{
			if( !( currentViewSetting.DrawDependencyLeftCurve && DrawDependencyLeftCurve ) &&
				!( currentViewSetting.DrawDependencyInsideCurve && DrawDependencyInsideCurve ) &&
				!( currentViewSetting.DrawDependencyInsideLine && DrawDependencyInsideLine )
			) {
				return;
			}

			List<Dependency> deps = new List<Dependency>();

			for( ulong id = drawIdFrom; id <= drawIdTo; id++ ) {
				Insn insn = GetInsn( id );
				if( insn == null )
					continue;

				var prodConsList = new[] { insn.Producers, insn.Consumers };
				foreach( var insnList in prodConsList ) {
					foreach( Insn.Relation rel in insnList ) {
						Insn relInsn = GetInsn( rel.id );
						if( relInsn == null )
							continue;

						ulong prodID = Math.Min( id, rel.id );
						ulong consID = Math.Max( id, rel.id );
						if( !( viewIdFrom > consID || viewIdTo < prodID ) ) {
							Dependency dep = new Dependency( prodID, consID, rel.type );
							if( !deps.Contains( dep ) ) {
								deps.Add( dep );
							}
						}
					}

				}
			}

			foreach( Dependency dep in deps ) {
				DrawDependencyArrow( g, dep.Producer, dep.Consumer, dep.Type );
			}
		}

		private void DrawDependencyArrow( Graphics g, ulong prodid, ulong consid, int type )
		{
			Insn prodinsn = GetInsn( prodid );
			Insn consinsn = GetInsn( consid );
			if( prodinsn == null || consinsn == null )
				return;

			float prodBaseY = coordinateSystem.YAtInsnId( prodid ) + CellMarginHeight + coordinateSystem.Cell.Height * 0.5f;
			float consBaseY = coordinateSystem.YAtInsnId( consid ) + CellMarginHeight + coordinateSystem.Cell.Height * 0.5f;
			float prody = prodBaseY;
			float consy = consBaseY;

			if( type >= dependencyArrows.Count ) {
				type = 0;
			}

			DependencyArrow arrow = dependencyArrows[ type ];
			Brush dependencyArrowBrush = arrow.brush;
			Pen   dependencyArrowPen   = arrow.pen;

			if( currentViewSetting.DrawDependencyLeftCurve && DrawDependencyLeftCurve ) {
				float prodx = coordinateSystem.XAtCycle( prodinsn.StartCycle );
				float consx = coordinateSystem.XAtCycle( consinsn.StartCycle );

				float dy = consy - prody;
				float left = Math.Min( prodx, consx ) - coordinateSystem.Grid.Width;
				PointF[] pts = new PointF[ 4 ];
				pts[ 0 ] = new PointF( prodx, prody );
				pts[ 1 ] = new PointF( left, prody );
				pts[ 2 ] = new PointF( left, consy );
				pts[ 3 ] = new PointF( consx, consy );
				DrawArrowhead( g, dependencyArrowBrush, pts[ 3 ], new PointF( currentViewSetting.DependencyArrowheadLength, 0 ), 0.8f );
				g.DrawBezier( dependencyArrowPen, pts[ 0 ], pts[ 1 ], pts[ 2 ], pts[ 3 ] );
			}

			if( ( currentViewSetting.DrawDependencyInsideCurve && DrawDependencyInsideCurve )  || 
				( currentViewSetting.DrawDependencyInsideLine && DrawDependencyInsideLine )
			) {
				float prodx = 0;  // prodx, consyが初期化されていないと文句を言われるので……
				float consx = 0;

				// Find an execution stage from a producer and 
				// decide the beginning point of a dependency line.
				{
					bool  found = false;
					int prodLane = 0;
					float prodLaneY = prodBaseY;
					foreach( var segment in prodinsn.StageSegments ) {
						if( prodLane != 0 && config.DrawInSeparateLane[ prodLane ] ) {
							prodLaneY += coordinateSystem.Cell.Height;
							prody = prodLaneY;
						}
						else {
							prody = prodBaseY;
						}

						List< Insn.Stage > prodStages = segment;	// 0 is a normal stage segment.
						for( int i = prodStages.Count - 1; i >= 0; i-- ) {
							Insn.Stage stage = prodStages[ i ];
							if( IsExecStageId( prodLane, stage.Id ) ) {
								prodx =
									coordinateSystem.XAtCycle( prodinsn.StartCycle + stage.EndRelCycle ) -
									coordinateSystem.Grid.Width * 0.2f;
								if( stage.Length == 0 ) {
									prodx += coordinateSystem.Grid.Width;
								}
								found = true;
								break;
							}
						}
						if( found ) {
							break;
						}
						prodLane++;
					}
					if( !found ) {
						return;
					}
				}

				// Find an execution stage from a consumer.
				// decide the end point of a dependency line.
				{
					bool found = false;
					int consLane = 0;
					float consLaneY = consBaseY;
					foreach( var segment in consinsn.StageSegments ) {
						if( consLane != 0 && config.DrawInSeparateLane[ consLane ] ) {
							consLaneY += coordinateSystem.Cell.Height;
							consy = consLaneY;
						}
						else {
							consy = consBaseY;
						}

						List< Insn.Stage > consStages = segment;
						for( int i = consStages.Count - 1; i >= 0; i-- ) {
							Insn.Stage stage = consStages[ i ];
							if( IsExecStageId( consLane, stage.Id ) ) {
								consx =
									coordinateSystem.XAtCycle( consinsn.StartCycle + stage.BeginRelCycle ) +
									coordinateSystem.Grid.Width * 0.2f;
								found = true;
								break;
							}
						}
						if( found ) {
							break;
						}
						consLane++;
					}
					if( !found ) {
						return;
					}
				}

				// Draw a dependency line.
				if( currentViewSetting.DrawDependencyInsideCurve && DrawDependencyInsideCurve ) {
					float xDiff = ( consx - prodx ) * 0.6f;
					float yDiff = 0;
					{

						PointF[] pts = new PointF[ 4 ];
						pts[ 0 ] = new PointF( prodx, prody );
						pts[ 1 ] = new PointF( prodx + xDiff, prody + yDiff );
						pts[ 2 ] = new PointF( consx - xDiff, consy - yDiff );
						pts[ 3 ] = new PointF( consx, consy );
						g.DrawBezier( dependencyArrowPen, pts[ 0 ], pts[ 1 ], pts[ 2 ], pts[ 3 ] );
					}

					// Draw a dependency arrow head.
					{
						PointF arrowVector =
							new PointF(
								( consx - prodx ) * 3,
								consy - prody
							);
						float norm = (float)Math.Sqrt( arrowVector.X * arrowVector.X + arrowVector.Y * arrowVector.Y );
						float f = currentViewSetting.DependencyArrowheadLength / norm;
						arrowVector.X *= f;
						arrowVector.Y *= f;
						DrawArrowhead(
							g,
							dependencyArrowBrush,
							new PointF( consx, consy ),
							arrowVector,
							0.8f
						);
					}
				}

				if( currentViewSetting.DrawDependencyInsideLine && DrawDependencyInsideLine ) {
					g.DrawLine( dependencyArrowPen, prodx, prody, consx, consy );

					PointF v = new PointF( consx - prodx, consy - prody );
					float norm = (float)Math.Sqrt( v.X * v.X + v.Y * v.Y );
					float f = currentViewSetting.DependencyArrowheadLength / norm;
					v.X *= f;
					v.Y *= f;
					DrawArrowhead( g, dependencyArrowBrush, new PointF( consx, consy ), v, 0.8f );
				}
			}
		}

		/// <summary>
		/// 矢印の先端を描画する
		/// </summary>
		/// <param name="g">描画先のGraphics</param>
		/// <param name="brush">描画に使用するbrush</param>
		/// <param name="target">やじりの先端</param>
		/// <param name="v">向きと高さを指定するベクトル</param>
		/// <param name="shape">底辺と高さの比</param>
		private void DrawArrowhead( Graphics g, Brush brush, PointF target, PointF v, float shape )
		{
			PointF[] pts = new PointF[ 3 ];

			pts[ 0 ] = target;
			pts[ 1 ] = new PointF( target.X - v.X - v.Y * 0.5f * shape, target.Y - v.Y + v.X * 0.5f * shape );
			pts[ 2 ] = new PointF( target.X - v.X + v.Y * 0.5f * shape, target.Y - v.Y - v.X * 0.5f * shape );

			g.FillPolygon( brush, pts );
		}

		// TIDからバックグラウンドの色を取得
		private Brush GetBackGroundBrush( int tid )
		{
			if( tid < backGroundBrushes.Count && backGroundBrushes[ tid ] != null ) {
				return backGroundBrushes[ tid ];
			}
			for( int i = backGroundBrushes.Count; i <= tid; i++ ) {
				backGroundBrushes.Add( null );
			}

			Color[] baseTbl = {
				Color.White,
				Color.AliceBlue,
				Color.LightYellow,
				Color.Honeydew,
				Color.LavenderBlush,
				Color.Lavender,
				Color.Bisque,
			};

			int midPos = tid / baseTbl.Length;
			int index  = tid % baseTbl.Length;

			Color threadColor;
			if( midPos != 0 ) {
				// ベースのテーブルから中間色を作る
				int next = ( index + 1 ) % baseTbl.Length;
				int denom = 4;
				midPos %= denom;
				threadColor = Color.FromArgb(
					( baseTbl[ index ].R * midPos + baseTbl[ next ].R * ( denom - midPos ) ) / denom,
					( baseTbl[ index ].G * midPos + baseTbl[ next ].G * ( denom - midPos ) ) / denom,
					( baseTbl[ index ].B * midPos + baseTbl[ next ].B * ( denom - midPos ) ) / denom
				);

			}
			else {
				threadColor = baseTbl[ index ];
			}

			backGroundBrushes[ tid ] = new SolidBrush( threadColor );
			return backGroundBrushes[ tid ];
		}

		// バックグラウンドの描画
		private void DrawMainViewBackGround( Graphics g, Rectangle backGroundRect )
		{
			if( config.MainViewTransparent ) {
				g.Clear( panelMainView.BackColor );
				return;
			}

			float cellHeight = coordinateSystem.Grid.Height;

			int bottom = backGroundRect.Bottom + (int)cellHeight;
			if( !IsYVisible( backGroundRect.Bottom - 1 ) ) {
				bottom = GetVisibleBottom();

				// Draw the background out of valid insn range
				Rectangle rect = 
					new Rectangle(
						backGroundRect.Left, bottom,
						backGroundRect.Width, backGroundRect.Bottom - bottom
					);
				g.FillRectangle( invalidBackGroundBrush, rect );

			}

			for( float y = backGroundRect.Top; y < bottom; y += cellHeight ) {
				ulong insnID = coordinateSystem.InsnIdAtY( (int)y );
				int top = coordinateSystem.YAtInsnId( insnID );
				Insn insn = GetInsn( insnID );
				if( insn == null ) {
					continue;
				}

				ulong tid = insn.Tid;
				Rectangle rect = 
					new Rectangle(
						backGroundRect.Left, top,
						backGroundRect.Width, (int)cellHeight
					);

				Brush brush = GetBackGroundBrush( (int)tid );
				g.FillRectangle( brush, rect );
			}
		}

		// メインビューの描画
		private void DrawMainView( Graphics g, Rectangle rect )
		{
			
			
			// Results of FillRectangle may be corrupt in an AntiAlias mode,
			// thus set a None mode for drawing a background and stages.
			g.SmoothingMode = SmoothingMode.None;	

			DrawMainViewBackGround( g, rect );

			// Resume
			g.SmoothingMode = SmoothingMode.AntiAlias;

			if( !loaded ) {
				return;
			}

			// 命令の描画
			if( ZoomedOut ) {
				// ズームアウト時
				// 枠線のみ描画
				for( int y = rect.Top; y <= rect.Bottom; y++ ) {
					ulong viewidfrom = coordinateSystem.InsnIdAtY( y );
					ulong viewidto = coordinateSystem.InsnIdAtY( y + 1 );
					if( viewidto != coordinateSystem.IdTo )
						viewidto -= 1;
					long mincycle = long.MaxValue;
					long maxcycle = long.MinValue;
					bool insnpresent = false;
					for( ulong id = viewidfrom; id <= viewidto; id++ ) {
						Insn insn = GetInsn( id );
						if( insn == null )
							continue;
						mincycle = Math.Min( mincycle, insn.StartCycle );
						maxcycle = Math.Max( maxcycle, insn.EndCycle );
						insnpresent = true;
					}
					if( insnpresent ) {
						int x1 = coordinateSystem.XAtCycle( mincycle );
						int x2 = coordinateSystem.XAtCycle( maxcycle );

						Pen pen;
						if( config.ColorScheme == ColorScheme.Default ) {
							pen = Pens.DimGray;
						}
						else if( config.ColorScheme == ColorScheme.Blue ) {
							pen = Pens.Cyan;
						}
						else {
							pen = Pens.Salmon;
						}
						g.DrawLine( pen, x1, y, x2, y );

					}
					if( viewidto == coordinateSystem.IdTo )
						break;
				}
				long cycle = coordinateSystem.CycleAtX( rect.Left );
			}
			else {
				// 最大ズーム時
				long cycle = coordinateSystem.CycleAtX( rect.Left );
				ulong viewIDFrom = coordinateSystem.InsnIdAtY( rect.Top );
				ulong viewIDTo = coordinateSystem.InsnIdAtY( rect.Bottom );

				for( ulong id = viewIDFrom; id <= viewIDTo; id++ ) {
					Insn insn = GetInsn( id );
					if( insn != null ) {
						DrawInsn( g, insn );
					}
				}


				// depViewIDFrom から depViewIDTo の間にある命令の依存関係のみが，更新領域に関わる．
				// depViewIDFrom：viewIDFrom の命令を横切る依存関係のうち，もっとも上（ID が小さい）命令
				// depViewIDTo  ：viewIDTo の命令を横切る依存関係のうち，もっとも下（ID が大きい）命令
				ulong depViewIDFrom = GetDependencyRange( viewIDFrom ).GetFront();
				ulong depViewIDTo   = GetDependencyRange( viewIDTo ).GetBack();
				DrawDependency(
					g,
					depViewIDFrom,
					depViewIDTo,
					viewIDFrom,
					viewIDTo
				);
			}
		}

		private void DrawLabelView( Graphics g, Rectangle rect )
		{
			//g.TranslateTransform( 0, -panelMainView.ScrollPosition.Y );
			//rect.Offset( 0, +panelMainView.ScrollPosition.Y );

			g.Clear( panelLabel.BackColor );

			// 上下+1の範囲から描画しないと端が化ける
			ulong viewidfrom = coordinateSystem.InsnIdAtY( rect.Top + panelMainView.ScrollPosition.Y );
			ulong viewidto = coordinateSystem.InsnIdAtY( rect.Bottom + panelMainView.ScrollPosition.Y ) + 1;
			if( viewidfrom > 0 )
				viewidfrom--;

			if( ZoomedOut ) {
			}
			else {
				for( ulong id = viewidfrom; id <= viewidto; id++ ) {
					if( !IsVisibleInsnId( id ) ) {
						continue;
					}
					Insn insn = GetInsn( id );
					string name = insn == null ? "" : insn.Name;
					ulong rid = insn == null ? 0 : insn.Rid;
					ulong tid = insn == null ? 0 : insn.Tid;
					ulong gsid = insn == null ? 0 : insn.Gsid;

					string s = string.Format(
						"{0}: {1} (T{2} : R{3}) : {4}",
						( id - coordinateSystem.IdFrom ).ToString( "D3" ),	// line
						gsid.ToString( "D3" ),			// gsid
						tid,							// tid
						rid.ToString( "D3" ),			// rid
						name
					);
					SizeF size = g.MeasureString( s, LabelFont );
					PointF p = 
						new Point( 
							0,
							coordinateSystem.YAtInsnId( id ) - panelMainView.ScrollPosition.Y
						);
					p.Y += CellMarginHeight / 2.0f + coordinateSystem.Cell.Height / 2.0f - size.Height / 2.0f;
					g.DrawString( s, LabelFont, Brushes.Black, p );
				}
			}
		}
	}
}