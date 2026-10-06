using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;

namespace VisualPascalABC
{
    // Keep WinForms menus, their nested drop-downs, and context menus in sync with the editor theme.
    public sealed class ToolStripThemeManager
    {
        private static readonly Color DarkBackground = Color.FromArgb(37, 37, 38);
        private static readonly Color DarkText = Color.FromArgb(230, 230, 230);
        private readonly ToolStripRenderer darkRenderer = new DarkToolStripRenderer();
        private readonly Dictionary<ToolStrip, ToolStripRenderMode> originalModes =
            new Dictionary<ToolStrip, ToolStripRenderMode>();
        private readonly Dictionary<ToolStrip, ToolStripRenderer> originalRenderers =
            new Dictionary<ToolStrip, ToolStripRenderer>();
        private readonly HashSet<ToolStripDropDownItem> openingHandlers = new HashSet<ToolStripDropDownItem>();
        private bool dark;

        public void Apply(bool dark, params ToolStrip[] strips)
        {
            this.dark = dark;
            foreach (ToolStrip strip in strips)
                ApplyStrip(strip);
        }

        private void ApplyStrip(ToolStrip strip)
        {
            if (strip == null)
                return;
            if (!originalModes.ContainsKey(strip))
            {
                originalModes.Add(strip, strip.RenderMode);
                if (strip.RenderMode == ToolStripRenderMode.Custom)
                    originalRenderers.Add(strip, strip.Renderer);
            }

            if (dark)
                strip.Renderer = darkRenderer;
            else if (originalModes[strip] == ToolStripRenderMode.Custom)
                strip.Renderer = originalRenderers[strip];
            else
                strip.RenderMode = originalModes[strip];

            strip.BackColor = dark ? DarkBackground : Color.Empty;
            strip.ForeColor = dark ? DarkText : Color.Empty;
            foreach (ToolStripItem item in strip.Items)
            {
                item.BackColor = dark ? DarkBackground : Color.Empty;
                item.ForeColor = dark ? DarkText : Color.Empty;
                ToolStripDropDownItem dropDownItem = item as ToolStripDropDownItem;
                if (dropDownItem == null)
                    continue;
                if (openingHandlers.Add(dropDownItem))
                    dropDownItem.DropDownOpening += OnDropDownOpening;
                if (dropDownItem.HasDropDownItems)
                    ApplyStrip(dropDownItem.DropDown);
            }
            strip.Invalidate();
        }

        private void OnDropDownOpening(object sender, EventArgs e)
        {
            ApplyStrip(((ToolStripDropDownItem)sender).DropDown);
        }

        private sealed class DarkToolStripRenderer : ToolStripProfessionalRenderer
        {
            private static readonly Color Highlight = Color.FromArgb(62, 62, 64);
            private static readonly Color Border = Color.FromArgb(80, 80, 80);

            public DarkToolStripRenderer() : base(new DarkColorTable()) { }

            protected override void OnRenderToolStripBackground(ToolStripRenderEventArgs e)
            {
                using (Brush brush = new SolidBrush(DarkBackground))
                    e.Graphics.FillRectangle(brush, e.AffectedBounds);
            }

            protected override void OnRenderImageMargin(ToolStripRenderEventArgs e)
            {
                using (Brush brush = new SolidBrush(DarkBackground))
                    e.Graphics.FillRectangle(brush, e.AffectedBounds);
            }

            protected override void OnRenderMenuItemBackground(ToolStripItemRenderEventArgs e)
            {
                Rectangle bounds = new Rectangle(Point.Empty, e.Item.Size);
                Color background = e.Item.Selected || e.Item.Pressed ? Highlight : DarkBackground;
                using (Brush brush = new SolidBrush(background))
                    e.Graphics.FillRectangle(brush, bounds);
                if (e.Item.Selected || e.Item.Pressed)
                {
                    using (Pen pen = new Pen(Border))
                        e.Graphics.DrawRectangle(pen, 0, 0, bounds.Width - 1, bounds.Height - 1);
                }
            }

            protected override void OnRenderToolStripBorder(ToolStripRenderEventArgs e)
            {
                if (e.ToolStrip is ToolStripDropDown)
                {
                    using (Pen pen = new Pen(Border))
                        e.Graphics.DrawRectangle(pen, 0, 0,
                            e.ToolStrip.Width - 1, e.ToolStrip.Height - 1);
                }
            }
        }

        private sealed class DarkColorTable : ProfessionalColorTable
        {
            private static readonly Color Highlight = Color.FromArgb(62, 62, 64);
            private static readonly Color Border = Color.FromArgb(80, 80, 80);

            public DarkColorTable()
            {
                UseSystemColors = false;
            }

            public override Color MenuStripGradientBegin { get { return DarkBackground; } }
            public override Color MenuStripGradientEnd { get { return DarkBackground; } }
            public override Color ToolStripDropDownBackground { get { return DarkBackground; } }
            public override Color ToolStripGradientBegin { get { return DarkBackground; } }
            public override Color ToolStripGradientMiddle { get { return DarkBackground; } }
            public override Color ToolStripGradientEnd { get { return DarkBackground; } }
            public override Color StatusStripGradientBegin { get { return DarkBackground; } }
            public override Color StatusStripGradientEnd { get { return DarkBackground; } }
            public override Color ImageMarginGradientBegin { get { return DarkBackground; } }
            public override Color ImageMarginGradientMiddle { get { return DarkBackground; } }
            public override Color ImageMarginGradientEnd { get { return DarkBackground; } }
            public override Color MenuItemSelected { get { return Highlight; } }
            public override Color MenuItemSelectedGradientBegin { get { return Highlight; } }
            public override Color MenuItemSelectedGradientEnd { get { return Highlight; } }
            public override Color MenuItemPressedGradientBegin { get { return Highlight; } }
            public override Color MenuItemPressedGradientMiddle { get { return Highlight; } }
            public override Color MenuItemPressedGradientEnd { get { return Highlight; } }
            public override Color MenuItemBorder { get { return Border; } }
            public override Color SeparatorDark { get { return Border; } }
            public override Color SeparatorLight { get { return Border; } }
            public override Color ButtonSelectedGradientBegin { get { return Highlight; } }
            public override Color ButtonSelectedGradientMiddle { get { return Highlight; } }
            public override Color ButtonSelectedGradientEnd { get { return Highlight; } }
            public override Color ButtonPressedGradientBegin { get { return Highlight; } }
            public override Color ButtonPressedGradientMiddle { get { return Highlight; } }
            public override Color ButtonPressedGradientEnd { get { return Highlight; } }
            public override Color CheckBackground { get { return Highlight; } }
            public override Color CheckSelectedBackground { get { return Highlight; } }
            public override Color CheckPressedBackground { get { return Highlight; } }
            public override Color GripDark { get { return Border; } }
            public override Color GripLight { get { return Highlight; } }
        }
    }
}
