using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace VisualPascalABC
{
    public static class WindowsCaptionTheme
    {
        // DWM caption and text colors are supported on Windows 11.
        private const int UseImmersiveDarkMode = 20;
        private const int CaptionColor = 35;
        private const int TextColor = 36;
        private const int DefaultColor = unchecked((int)0xFFFFFFFF);

        [DllImport("dwmapi.dll", PreserveSig = true)]
        private static extern int DwmSetWindowAttribute(IntPtr handle, int attribute, ref int value, int size);

        public static bool Apply(Form form, bool dark)
        {
            if (Environment.OSVersion.Platform != PlatformID.Win32NT ||
                Environment.OSVersion.Version.Build < 22000)
                return false;

            int enabled = dark ? 1 : 0;
            int caption = dark ? ColorRef(Color.FromArgb(37, 37, 38)) : DefaultColor;
            int text = dark ? ColorRef(Color.FromArgb(230, 230, 230)) : DefaultColor;
            try
            {
                IntPtr handle = form.Handle;
                int result = DwmSetWindowAttribute(handle, UseImmersiveDarkMode, ref enabled, sizeof(int));
                result |= DwmSetWindowAttribute(handle, CaptionColor, ref caption, sizeof(int));
                result |= DwmSetWindowAttribute(handle, TextColor, ref text, sizeof(int));
                return result == 0;
            }
            catch (DllNotFoundException) { return false; }
            catch (EntryPointNotFoundException) { return false; }
        }

        private static int ColorRef(Color color)
        {
            return color.R | color.G << 8 | color.B << 16;
        }
    }
}
