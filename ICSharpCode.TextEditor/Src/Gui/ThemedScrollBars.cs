using System;
using System.Drawing;
using System.Windows.Forms;

namespace ICSharpCode.TextEditor
{
    // Keep native scrollbar input handling while painting the dark editor palette.
    public sealed class ThemedHScrollBar : HScrollBar
    {
        private bool darkTheme;

        public bool DarkTheme
        {
            get { return darkTheme; }
            set
            {
                if (darkTheme == value)
                    return;
                darkTheme = value;
                SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint |
                    ControlStyles.OptimizedDoubleBuffer, value);
                Invalidate();
            }
        }

        public ThemedHScrollBar()
        {
            ValueChanged += delegate { if (darkTheme) Invalidate(); };
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            if (darkTheme)
                ThemedScrollBarPainter.Draw(e.Graphics, ClientRectangle, this, false);
            else
                base.OnPaint(e);
        }
    }

    public sealed class ThemedVScrollBar : VScrollBar
    {
        private bool darkTheme;

        public bool DarkTheme
        {
            get { return darkTheme; }
            set
            {
                if (darkTheme == value)
                    return;
                darkTheme = value;
                SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint |
                    ControlStyles.OptimizedDoubleBuffer, value);
                Invalidate();
            }
        }

        public ThemedVScrollBar()
        {
            ValueChanged += delegate { if (darkTheme) Invalidate(); };
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            if (darkTheme)
                ThemedScrollBarPainter.Draw(e.Graphics, ClientRectangle, this, true);
            else
                base.OnPaint(e);
        }
    }

    internal static class ThemedScrollBarPainter
    {
        private static readonly Color Track = Color.FromArgb(37, 37, 38);
        private static readonly Color Button = Color.FromArgb(45, 45, 48);
        private static readonly Color Thumb = Color.FromArgb(100, 100, 105);
        private static readonly Color Border = Color.FromArgb(63, 63, 70);
        private static readonly Color Arrow = Color.FromArgb(210, 210, 210);

        public static void Draw(Graphics graphics, Rectangle bounds, ScrollBar bar, bool vertical)
        {
            graphics.Clear(Track);
            int length = vertical ? bounds.Height : bounds.Width;
            int thickness = vertical ? bounds.Width : bounds.Height;
            int buttonSize = Math.Min(thickness, length / 2);
            int trackLength = Math.Max(0, length - 2 * buttonSize);
            Rectangle firstButton = vertical
                ? new Rectangle(0, 0, thickness, buttonSize)
                : new Rectangle(0, 0, buttonSize, thickness);
            Rectangle lastButton = vertical
                ? new Rectangle(0, length - buttonSize, thickness, buttonSize)
                : new Rectangle(length - buttonSize, 0, buttonSize, thickness);

            using (Brush buttonBrush = new SolidBrush(Button))
            using (Brush thumbBrush = new SolidBrush(Thumb))
            using (Pen borderPen = new Pen(Border))
            using (Pen arrowPen = new Pen(Arrow, 1.5f))
            {
                graphics.FillRectangle(buttonBrush, firstButton);
                graphics.FillRectangle(buttonBrush, lastButton);
                graphics.DrawRectangle(borderPen, 0, 0, bounds.Width - 1, bounds.Height - 1);
                if (trackLength > 0)
                {
                    int range = Math.Max(1, bar.Maximum - bar.Minimum + 1);
                    int thumbLength = Math.Min(trackLength, Math.Max(thickness, trackLength * bar.LargeChange / range));
                    int movableLength = trackLength - thumbLength;
                    int usableMaximum = Math.Max(bar.Minimum, bar.Maximum - bar.LargeChange + 1);
                    int currentValue = Math.Min(usableMaximum, Math.Max(bar.Minimum, bar.Value));
                    int thumbOffset = usableMaximum == bar.Minimum ? 0 :
                        (int)((long)(currentValue - bar.Minimum) * movableLength / (usableMaximum - bar.Minimum));
                    Rectangle thumb = vertical
                        ? new Rectangle(2, buttonSize + thumbOffset + 1, Math.Max(1, thickness - 4), Math.Max(1, thumbLength - 2))
                        : new Rectangle(buttonSize + thumbOffset + 1, 2, Math.Max(1, thumbLength - 2), Math.Max(1, thickness - 4));
                    graphics.FillRectangle(thumbBrush, thumb);
                }
                DrawArrow(graphics, arrowPen, firstButton, vertical, false);
                DrawArrow(graphics, arrowPen, lastButton, vertical, true);
            }
        }

        private static void DrawArrow(Graphics graphics, Pen pen, Rectangle button, bool vertical, bool forward)
        {
            int centerX = button.Left + button.Width / 2;
            int centerY = button.Top + button.Height / 2;
            int direction = forward ? 1 : -1;
            if (vertical)
            {
                graphics.DrawLine(pen, centerX - 3, centerY - direction, centerX, centerY + 2 * direction);
                graphics.DrawLine(pen, centerX, centerY + 2 * direction, centerX + 3, centerY - direction);
            }
            else
            {
                graphics.DrawLine(pen, centerX - direction, centerY - 3, centerX + 2 * direction, centerY);
                graphics.DrawLine(pen, centerX + 2 * direction, centerY, centerX - direction, centerY + 3);
            }
        }
    }
}
