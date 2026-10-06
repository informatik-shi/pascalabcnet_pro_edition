using System.Drawing;

namespace WeifenLuo.WinFormsUI.Docking
{
    // Shared by the VS2005 dock painters. The host refreshes its panes after changing this value.
    public static class DockPanelTheme
    {
        public static bool DarkMode { get; set; }

        public static readonly Color Surface = Color.FromArgb(37, 37, 38);
        public static readonly Color DocumentTab = Color.FromArgb(45, 45, 48);
        public static readonly Color EditorBackground = Color.FromArgb(30, 30, 30);
        public static readonly Color Border = Color.FromArgb(63, 63, 70);
        public static readonly Color Text = Color.FromArgb(230, 230, 230);
        public static readonly Color MutedText = Color.FromArgb(165, 165, 165);

        internal static readonly Brush SurfaceBrush = new SolidBrush(Surface);
        internal static readonly Brush DocumentTabBrush = new SolidBrush(DocumentTab);
        internal static readonly Brush EditorBackgroundBrush = new SolidBrush(EditorBackground);
        internal static readonly Brush TextBrush = new SolidBrush(Text);
        internal static readonly Pen BorderPen = new Pen(Border);
    }
}
