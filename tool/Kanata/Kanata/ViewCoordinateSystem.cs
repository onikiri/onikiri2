using System;
using System.Collections.Generic;
using System.Text;
using System.Drawing;

namespace Kanata
{
	class ViewCoordinateSystem
	{
		private ulong idFrom = 0;
		private ulong idTo = 1023;
		private long cycleFrom = 0;
		private long cycleTo = 0;

		private SizeF grid = new SizeF();
		private SizeF cell = new SizeF();

		public int MaxViewInsnCount
		{
			get { return 1024 * 1024;}
		}

		public ulong IdFrom
		{
			get { return idFrom; }
		}
		public ulong IdTo
		{
			get { return idTo; }
			// set { ViewInsnCount = value - idFrom + 1; }
		}
		public int ViewInsnCount
		{
			get { return (int)( idTo - idFrom + 1 ); }
		}
		public long CycleFrom
		{
			get { return cycleFrom; }
		}
		public long CycleTo
		{
			get { return cycleTo; }
		}
		public int ViewCycleCount
		{
			get { return (int)( cycleTo - cycleFrom + 1 ); }
		}

		public void SetInsnRange( ulong idFrom, ulong idTo )
		{
			this.idFrom = idFrom;
			this.idTo = idTo;
		}

		public void SetCycleRange( long cycleFrom, long cycleTo )
		{
			this.cycleFrom = cycleFrom;
			this.cycleTo = cycleTo;
		}

		public SizeF Grid
		{
			get { return grid; }
		}

		public void SetGrid( SizeF grid )
		{
			// need to copy
			this.grid = new SizeF( grid );
		}

		public SizeF Cell
		{
			get { return cell; }
		}

		public void SetCell( SizeF cell )
		{
			// need to copy
			this.cell = new SizeF( cell );
		}

		public Size ViewSize
		{
			get
			{
				int height = (int)Math.Ceiling( (double)Grid.Height * ViewInsnCount );
				int width = (int)Math.Ceiling( (double)Grid.Width * ViewCycleCount );
				return new Size( width, height );
			}
		}

		public int XAtCycle( long cycle )
		{
			return (int)( ( cycle - CycleFrom ) * Grid.Width );
		}

		public long CycleAtX( int x )
		{
			if( x < 0 )
				return CycleFrom;
			else
				return Math.Min( (int)( x / Grid.Width ) + CycleFrom, CycleTo );
		}

		public int YAtInsnId( ulong id )
		{
			return (int)( ( id - IdFrom ) * Grid.Height );
		}

		public ulong InsnIdAtY( int y )
		{
			if( y < 0 )
				return IdFrom;
			else
				return Math.Min( (ulong)( y / Grid.Height ) + IdFrom, IdTo );
		}

		public ulong InsnIdAtY_NoClip( int y )
		{
			return (ulong)( y / Grid.Height );
		}
	}
}
