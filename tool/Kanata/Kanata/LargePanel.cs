using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Diagnostics;

// Dejan's Weblog : Control.Width and Height are silently truncated to 32,767  
// http://www.jelovic.com/weblog/e127.htm

namespace Kanata
{
    /// <summary>
    /// Sizeの32767*32767ピクセルの制限を超えてスクロールを行う
    /// 
    /// スクロール範囲はVirtualSize
    /// スクロール位置はScrollPosition
    /// 
    /// VerticalScroll, HorizontalScrollは無効
    /// </summary>
    class LargePanel : ScrollableControl
    {

        // scrolling
        [DllImport("user32.dll")]
        private static extern int ScrollWindowEx(IntPtr hwnd, int dx, int dy, IntPtr prcScroll,
            IntPtr prcClip, IntPtr hrgnUpdate, IntPtr prcUpdate, uint flags);
        private const uint SW_INVALIDATE = 0x0002;

        //struct RECT
        //{
        //    public int left;
        //    public int top;
        //    public int right;
        //    public int bottom;
        //};

        [DllImport("user32.dll")]
        private static extern int SetScrollInfo(IntPtr hwnd, int fnBar, IntPtr lpsi, int fRedraw);
        [DllImport("user32.dll")]
        private static extern int GetScrollInfo(IntPtr hwnd, int fnBar, IntPtr lpsi);
        struct SCROLLINFO
        {
            public int cbSize;
            public uint fMask;
            public int nMin;
            public int nMax;
            public uint nPage;
            public int nPos;
            public int nTrackPos;
        };

        private const int SB_HORZ = 0;
        private const int SB_VERT = 1;
        private const int SB_CTL = 2;
        //private const int SB_BOTH = 3;

        private const int SB_LINEUP = 0;
        private const int SB_LINELEFT = 0;
        private const int SB_LINEDOWN = 1;
        private const int SB_LINERIGHT = 1;
        private const int SB_PAGEUP = 2;
        private const int SB_PAGELEFT = 2;
        private const int SB_PAGEDOWN = 3;
        private const int SB_PAGERIGHT = 3;
        private const int SB_THUMBPOSITION = 4;
        private const int SB_THUMBTRACK = 5;
        private const int SB_TOP = 6;
        private const int SB_LEFT = 6;
        private const int SB_BOTTOM = 7;
        private const int SB_RIGHT = 7;
        private const int SB_ENDSCROLL = 8;

        private const uint SIF_RANGE = 0x0001;
        private const uint SIF_PAGE = 0x0002;
        private const uint SIF_POS = 0x0004;
        private const uint SIF_DISABLENOSCROLL = 0x0008;
        private const uint SIF_TRACKPOS = 0x0010;
        private const uint SIF_ALL = (SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS);


        private Size virtualSize;

        //private bool disposed = false;

        // AutoScrollは無効にする
        public override bool AutoScroll
        {
            get { return true; }
            set { /* do nothing */ }
        }

        /// <summary>
        /// スクロール範囲を設定する．必要ならスクロール位置は切りつめられ，再描画される．
        /// </summary>
        public Size VirtualSize
        {
            get { return virtualSize; }
            set
            {
                virtualSize = value;
                UpdateScrollBars();
            }
        }

        /// <summary>
        /// 現在描画されている位置
        /// </summary>
        private Point scrollPosition = new Point();

        /// <summary>
        /// スクロール位置を設定する．再描画も行われる．
        /// </summary>
        public Point ScrollPosition
        {
            get { return scrollPosition; }
            set { ScrollWindowCore(value); }
        }

        private new VScrollProperties VerticalScroll {
            get { throw new NotSupportedException("VerticalScroll.get"); }
            set { throw new NotSupportedException("VerticalScroll.set"); }
        }
        private new HScrollProperties HorizontalScroll {
            get { throw new NotSupportedException("HorizontalScroll.get"); }
            set { throw new NotSupportedException("HorizontalScroll.set"); }
        }

        public LargePanel()
        {
			base.DoubleBuffered = true;
        }

        //protected override void Dispose(bool disposing)
        //{
        //    if (!disposed) {
        //        if (disposing) {
        //            base.Dispose(disposing);
        //        }
        //    }
        //    disposed = true;
        //}

        public int HScrollSmallChange = 16;
        public int HScrollLargeChange = 32;
        public int VScrollSmallChange = 16;
        public int VScrollLargeChange = 32;

        private const int WS_VSCROLL = 0x00200000;
        private const int WS_HSCROLL = 0x00100000;
        protected override CreateParams CreateParams
        {
            get
            {
                CreateParams p = base.CreateParams;
                p.Style |= WS_VSCROLL | WS_HSCROLL;
                return p;
            }
        }

        private const int WM_HSCROLL = 0x0114;
        private const int WM_VSCROLL = 0x0115;
        protected override void WndProc(ref Message m)
        {
            switch (m.Msg) {
            case WM_VSCROLL:
                ProcessWMScroll(SB_VERT, m.WParam.ToInt32() & 0xffff);
                return;
            case WM_HSCROLL:
                ProcessWMScroll(SB_HORZ, m.WParam.ToInt32() & 0xffff);
                return;
            default:
                break;
            }
            base.WndProc(ref m);
        }

        private void ProcessWMScroll(int fnBar, int nScrollCode)
        {
            int large_change;
            int small_change;
            ScrollOrientation orientation = new ScrollOrientation();
            switch (fnBar) {
            case SB_VERT:
                large_change = VScrollLargeChange;
                small_change = VScrollSmallChange;
                orientation = ScrollOrientation.VerticalScroll;
                break;
            case SB_HORZ:
                large_change = HScrollLargeChange;
                small_change = HScrollSmallChange;
                orientation = ScrollOrientation.HorizontalScroll;
                break;
            default:
                throw new ArgumentOutOfRangeException("fnBar");
            }

            SCROLLINFO si = new SCROLLINFO();
            unsafe {
                si.cbSize = sizeof(SCROLLINFO);
                si.fMask = SIF_ALL;
                GetScrollInfo(Handle, fnBar, new IntPtr(&si));
            }
            int oldValue = si.nPos;

            ScrollEventType type = new ScrollEventType();
            int newValue = si.nPos;
            switch (nScrollCode) {
            case SB_TOP:
                //case SB_LEFT:
                type = ScrollEventType.First;
                newValue = si.nMin;
                break;
            case SB_BOTTOM:
                //case SB_RIGHT:
                type = ScrollEventType.Last;
                newValue = si.nMax;
                break;
            case SB_LINEUP:
                //case SB_LINELEFT:
                type = ScrollEventType.SmallDecrement;
                newValue -= small_change;
                break;
            case SB_LINEDOWN:
                //case SB_LINERIGHT:
                type = ScrollEventType.SmallIncrement;
                newValue += small_change;
                break;
            case SB_PAGEUP:
                //case SB_PAGELEFT:
                type = ScrollEventType.LargeDecrement;
                newValue -= large_change;
                break;
            case SB_PAGEDOWN:
                //case SB_PAGERIGHT:
                type = ScrollEventType.LargeIncrement;
                newValue += large_change;
                break;
            case SB_THUMBPOSITION:
                type = ScrollEventType.ThumbPosition;
                newValue = si.nTrackPos;
                break;
            case SB_THUMBTRACK:
                type = ScrollEventType.ThumbTrack;
                newValue = si.nTrackPos;
                break;
            case SB_ENDSCROLL:
                type = ScrollEventType.EndScroll;
                break;
            default:
                throw new ArgumentOutOfRangeException("nScrollCode");
            }

            Point newPosition = scrollPosition;
            switch (orientation) {
            case ScrollOrientation.VerticalScroll:
                newPosition.Y = newValue;
                break;
            case ScrollOrientation.HorizontalScroll:
                newPosition.X = newValue;
                break;
            }
            ScrollWindowCore(newPosition);

            ScrollEventArgs se = new ScrollEventArgs(type, oldValue, newValue, orientation);
            OnScroll(se);
        }

        protected override void OnCreateControl()
        {
            base.OnCreateControl();
            UpdateScrollBars();
        }

        protected override void AdjustFormScrollbars(bool displayScrollbars)
        {
            //base.AdjustFormScrollbars(true);
        }

        // Paintの前に座標変換を行う
        protected override void OnPaint(PaintEventArgs e)
        {
            Graphics g = e.Graphics;
            Rectangle rect = e.ClipRectangle;

            g.TranslateTransform(-scrollPosition.X, -scrollPosition.Y);
            rect.Offset(scrollPosition);
            rect.Intersect(new Rectangle(0, 0, virtualSize.Width, virtualSize.Height));
            g.IntersectClip(rect);
            base.OnPaint(new PaintEventArgs(g, rect));
        }

        // ウィンドウのサイズが変えられたときに，ウィンドウのサイズに応じてスクロールバーを設定する
        protected override void SetBoundsCore(int x, int y, int width, int height, BoundsSpecified specified)
        {
            base.SetBoundsCore(x, y, width, height, specified);
            UpdateScrollBars();
        }

        /// <summary>
        /// ウィンドウをスクロールさせる
        /// </summary>
        /// <param name="newPosition">新しいスクロール位置</param>
        private void ScrollWindowCore(Point newPosition)
        {
            if (scrollPosition == newPosition)
                return;

            unsafe {
                // スクロールバーの位置を設定
                SCROLLINFO vsi = new SCROLLINFO();
                SCROLLINFO hsi = new SCROLLINFO();
                vsi.cbSize = hsi.cbSize = sizeof(SCROLLINFO);
                vsi.fMask = hsi.fMask = SIF_POS;
                GetScrollInfo(Handle, SB_VERT, new IntPtr(&vsi));
                GetScrollInfo(Handle, SB_HORZ, new IntPtr(&hsi));
                vsi.nPos = newPosition.Y;
                hsi.nPos = newPosition.X;
                SetScrollInfo(Handle, SB_VERT, new IntPtr(&vsi), 1);
                SetScrollInfo(Handle, SB_HORZ, new IntPtr(&hsi), 1);

                // 実際に設定された位置(範囲チェックされた位置)を取得
                newPosition = GetScrollPositionFromWindow();
            }

            int dx = newPosition.X - scrollPosition.X;
            int dy = newPosition.Y - scrollPosition.Y;
            unsafe {
                ScrollWindowEx(Handle, -dx, -dy, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, SW_INVALIDATE);
            }
            scrollPosition = newPosition;
        }

        /// <summary>
        /// VirtualSize, Sizeに従ってスクロールバーを設定する
        /// </summary>
        private void UpdateScrollBars()
        {
            unsafe {
                // Max，Pageを設定
                SCROLLINFO vsi = new SCROLLINFO();
                SCROLLINFO hsi = new SCROLLINFO();
                vsi.cbSize = hsi.cbSize = sizeof(SCROLLINFO);
                vsi.fMask = hsi.fMask = (SIF_RANGE | SIF_PAGE);
                vsi.nMin = hsi.nMin = 0;
                vsi.nMax = VirtualSize.Height;
                hsi.nMax = VirtualSize.Width;
                vsi.nPage = (uint)Size.Height;
                hsi.nPage = (uint)Size.Width;
                SetScrollInfo(Handle, SB_VERT, new IntPtr(&vsi), 1);
                SetScrollInfo(Handle, SB_HORZ, new IntPtr(&hsi), 1);

                // (Posの位置が範囲外になったことによって)Posが変化していればスクロールさせる
                Point newPosition = GetScrollPositionFromWindow();
                ScrollWindowCore(newPosition);
            }
        }

        private Point GetScrollPositionFromWindow()
        {
            unsafe {
                SCROLLINFO vsi = new SCROLLINFO();
                SCROLLINFO hsi = new SCROLLINFO();
                vsi.cbSize = hsi.cbSize = sizeof(SCROLLINFO);
                vsi.fMask = hsi.fMask = SIF_POS;
                // スクロールバーが表示されていないときはnPosを0にする
                if (GetScrollInfo(Handle, SB_VERT, new IntPtr(&vsi)) == 0)
                    vsi.nPos = 0;
                if (GetScrollInfo(Handle, SB_HORZ, new IntPtr(&hsi)) == 0)
                    hsi.nPos = 0;
                return new Point(hsi.nPos, vsi.nPos);
            }
        }

        public MouseEventArgs TranslateMouseEventArgs(MouseEventArgs e)
        {
            return new MouseEventArgs(e.Button, e.Clicks, e.X + ScrollPosition.X, e.Y + ScrollPosition.Y, e.Delta);
        }

        protected override void OnMouseMove(MouseEventArgs e)
        {
            base.OnMouseMove(TranslateMouseEventArgs(e));
        }

        protected override void OnMouseDown(MouseEventArgs e)
        {
            base.OnMouseDown(TranslateMouseEventArgs(e));
        }

        protected override void OnMouseUp(MouseEventArgs e)
        {
            base.OnMouseUp(TranslateMouseEventArgs(e));
        }

        protected override void OnMouseClick(MouseEventArgs e)
        {
            base.OnMouseClick(TranslateMouseEventArgs(e));
        }

        protected override void OnMouseDoubleClick(MouseEventArgs e)
        {
            base.OnMouseDoubleClick(TranslateMouseEventArgs(e));
        }
        protected override void OnMouseWheel(MouseEventArgs e)
        {
            base.OnMouseWheel(TranslateMouseEventArgs(e));
        }
    }
}
