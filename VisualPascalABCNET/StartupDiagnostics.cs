using System;
using System.IO;
using System.Windows.Forms;

namespace VisualPascalABC
{
    internal static class StartupDiagnostics
    {
        private static readonly object Sync = new object();
        private static string logFile;

        internal static void Initialize()
        {
            try
            {
                var basePath = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
                logFile = Path.Combine(basePath, "abc.pro", "Logs", "ide-startup.log");
                Directory.CreateDirectory(Path.GetDirectoryName(logFile));
            }
            catch
            {
                logFile = Path.Combine(Path.GetTempPath(), "abc-pro-ide-startup.log");
            }
            Write("IDE started; executable: " + Application.ExecutablePath);
            AppDomain.CurrentDomain.UnhandledException += (sender, eventArgs) =>
                Report(eventArgs.ExceptionObject as Exception ??
                    new Exception(Convert.ToString(eventArgs.ExceptionObject)));
            Application.ThreadException += (sender, eventArgs) => Report(eventArgs.Exception);
        }

        internal static void Write(string message)
        {
            try
            {
                lock (Sync)
                    File.AppendAllText(logFile,
                        DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss") + " " + message + Environment.NewLine);
            }
            catch
            {
                // Diagnostics must never prevent the IDE from starting.
            }
        }

        internal static void Report(Exception error)
        {
            Write("Unhandled error: " + error);
            try
            {
                MessageBox.Show(error + Environment.NewLine + Environment.NewLine +
                    "Журнал запуска: " + logFile, "abc.pro", MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
            catch
            {
            }
        }
    }
}
