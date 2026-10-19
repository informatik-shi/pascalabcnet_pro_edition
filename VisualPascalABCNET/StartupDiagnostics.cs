using System;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Windows.Forms;
using ABCPro.Diagnostics;

namespace VisualPascalABC
{
    internal static class StartupDiagnostics
    {
        private static int reporting;
        internal static void Write(string message) { DiagnosticLog.Write(message); }
        [MethodImpl(MethodImplOptions.NoInlining)]
        internal static void RegisterUiHandler()
        {
            Application.SetUnhandledExceptionMode(UnhandledExceptionMode.CatchException);
            Application.ThreadException += (sender, e) => {
                Report(e.Exception);
                Application.Exit();
            };
        }
        internal static void Report(Exception error)
        {
            DiagnosticLog.Failure(error);
            Environment.ExitCode = 1;
            if (Interlocked.Exchange(ref reporting, 1) != 0) return;
            try { FailureDialog.Show(error); } catch { }
        }
    }

    // Form1 and plugin type loading occurs only inside the protected call.
    internal static class DiagnosticBootstrap
    {
        [STAThread]
        private static void Main(string[] args)
        {
            DiagnosticLog.Initialize("ide");
            AppDomain.CurrentDomain.UnhandledException += (sender, e) => StartupDiagnostics.Report(
                e.ExceptionObject as Exception ?? new Exception(Convert.ToString(e.ExceptionObject)));
            try {
                DiagnosticLog.DescribeRuntime(System.IO.Path.GetFullPath(System.IO.Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "..")));
                StartupDiagnostics.RegisterUiHandler();
                StartIde(args);
                DiagnosticLog.Write("IDE ended; exit code=" + Environment.ExitCode);
            } catch (Exception e) { StartupDiagnostics.Report(e); }
        }
        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void StartIde(string[] args) { VisualPascalABCProgram.Run(args); }
    }
}
