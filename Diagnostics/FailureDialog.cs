using System;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Windows.Forms;

namespace ABCPro.Diagnostics
{
    internal static class FailureDialog
    {
        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void Show(Exception error)
        {
            try {
                var path = DiagnosticLog.FilePath;
                var text = "abc.pro: произошла ошибка на этапе «" + DiagnosticLog.Stage + "».\r\n\r\n" + error.Message +
                    "\r\n\r\n" + (path == null ? "Не удалось записать отчёт в пользовательские папки.\r\n" + error :
                    "Передайте этот файл для разбора:\r\n" + path + "\r\n\r\nОткрыть отчёт?\r\nПуть можно скопировать из этого окна клавишами Ctrl+C.");
                if (MessageBox.Show(text, "abc.pro — диагностический отчёт", path == null ? MessageBoxButtons.OK : MessageBoxButtons.YesNo,
                    MessageBoxIcon.Error) == DialogResult.Yes && path != null) {
                    try { Process.Start(new ProcessStartInfo(path) { UseShellExecute = true }); }
                    catch (Exception e) { DiagnosticLog.Write("Could not open report: " + e); }
                }
                DiagnosticLog.Write("Failure dialog displayed");
            } catch (Exception e) { DiagnosticLog.Write("Failure dialog unavailable: " + e); }
        }
    }
}
