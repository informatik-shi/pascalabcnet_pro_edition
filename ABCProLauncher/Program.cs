using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using ABCPro.Diagnostics;

namespace ABCProLauncher
{
    internal static class Program
    {
        [STAThread]
        private static int Main(string[] args)
        {
            DiagnosticLog.Initialize("launcher");
            try
            {
                var root = AppDomain.CurrentDomain.BaseDirectory.TrimEnd(
                    Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
                var bin = Path.Combine(root, "bin");
                var ide = Path.Combine(bin, "PascalABCNET.exe");
                var dotnet = Path.Combine(root, "dotnet");
                var python = Path.Combine(root, "python", "python.exe");
                var notebook = Path.Combine(root, "notebook", "PascalABCNotebook.dll");
                DiagnosticLog.SetStage("package and runtime validation");
                DiagnosticLog.DescribeRuntime(root);
                foreach (var required in new[] { ide, Path.Combine(dotnet, "dotnet.exe"), python, notebook })
                    if (!File.Exists(required))
                        throw new FileNotFoundException("Неполная установка abc.pro", required);

                var portable = args.Length > 0 && args[0] == "--portable";
                if (portable) args = args.Skip(1).ToArray();
                // Installer smoke tests can validate the package without starting the IDE.
                if (args.Length == 1 && args[0] == "--self-test")
                    return 0;

                var data = Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                    "abc.pro");
                var work = portable ? Path.Combine(root, "Work") : Path.Combine(data, "Work");
                DiagnosticLog.SetStage("user work directory");
                try { DiagnosticLog.ProbeDirectory("Work directory", work); }
                catch {
                    if (!portable) throw;
                    work = Path.Combine(data, "Work");
                    DiagnosticLog.ProbeDirectory("Fallback work directory", work);
                }
                Directory.CreateDirectory(Path.Combine(work, "Output"));
                Directory.CreateDirectory(Path.Combine(work, "Matplotlib"));
                CopyStarterDirectory(Path.Combine(root, "Templates", "Samples"),
                    Path.Combine(work, "Samples"));
                CopyStarterDirectory(Path.Combine(root, "Templates", "Files"),
                    Path.Combine(work, "Files"));
                Environment.SetEnvironmentVariable("PABC_WORK_DIRECTORY", work);
                DiagnosticLog.Write("Settings file: " + Path.Combine(work, "PascalABCNET.ini") + "; uses checked work directory");

                Environment.SetEnvironmentVariable("DOTNET_ROOT", dotnet);
                Environment.SetEnvironmentVariable("DOTNET_ROOT_X64", dotnet);
                Environment.SetEnvironmentVariable("PABCNET_ROOT", root);
                Environment.SetEnvironmentVariable("PABC_PYTHON_EXE", python);
                Environment.SetEnvironmentVariable("MPLCONFIGDIR",
                    Path.Combine(work, "Matplotlib"));
                Environment.SetEnvironmentVariable("PATH", dotnet + Path.PathSeparator +
                    Environment.GetEnvironmentVariable("PATH"));

                var start = new ProcessStartInfo(ide, string.Join(" ", args.Select(QuoteArgument)))
                {
                    WorkingDirectory = bin,
                    UseShellExecute = false
                };
                DiagnosticLog.SetStage("Process.Start IDE");
                Environment.SetEnvironmentVariable("PABC_DIAGNOSTIC_LAUNCH_LOG", DiagnosticLog.FilePath);
                using (var child = Process.Start(start)) {
                    DiagnosticLog.SetStage("monitor IDE process");
                    DiagnosticLog.Write("IDE PID=" + child.Id);
                    child.WaitForExit();
                    DiagnosticLog.Write("IDE exit code=" + child.ExitCode + " (0x" + child.ExitCode.ToString("X8") + ")");
                    if (child.ExitCode != 0) {
                        // Native/loader crashes may provide no managed exception at all.
                        var error = new InvalidOperationException("Процесс IDE завершился с кодом " + child.ExitCode +
                            " (0x" + child.ExitCode.ToString("X8") + "). Причина по одному коду не установлена. " +
                            "Также передайте ide-отчёт этого запуска из папки журналов, если он создан.");
                        DiagnosticLog.Failure(error);
                        var notified = false;
                        try {
                            var report = DiagnosticLog.RelatedReport("ide");
                            if (report != null) {
                                var detail = File.ReadAllText(report);
                                notified = detail.Contains("Failure dialog displayed");
                                DiagnosticLog.Write("IDE report: " + report + Environment.NewLine + detail);
                            }
                        } catch (Exception readError) { DiagnosticLog.Write("Could not include IDE report: " + readError); }
                        if (notified) DiagnosticLog.Write("Failure dialog displayed by IDE");
                        else FailureDialog.Show(error);
                        return 1;
                    }
                    return 0;
                }
            }
            catch (Exception error)
            {
                DiagnosticLog.Failure(error);
                FailureDialog.Show(error);
                return 1;
            }
            finally { DiagnosticLog.Write("Launcher completed"); }
        }

        private static void CopyStarterDirectory(string source, string destination)
        {
            if (!Directory.Exists(source) || Directory.Exists(destination))
                return;
            Directory.CreateDirectory(destination);
            foreach (var file in Directory.GetFiles(source))
                File.Copy(file, Path.Combine(destination, Path.GetFileName(file)));
            foreach (var directory in Directory.GetDirectories(source))
                CopyStarterDirectory(directory,
                    Path.Combine(destination, Path.GetFileName(directory)));
        }

        private static string QuoteArgument(string value)
        {
            if (value.Length > 0 && value.IndexOfAny(new[] { ' ', '\t', '"' }) < 0)
                return value;
            var text = new StringBuilder("\"");
            var slashes = 0;
            foreach (var ch in value)
            {
                if (ch == '\\')
                {
                    slashes++;
                    continue;
                }
                if (ch == '"')
                {
                    text.Append('\\', slashes * 2 + 1).Append('"');
                    slashes = 0;
                    continue;
                }
                text.Append('\\', slashes).Append(ch);
                slashes = 0;
            }
            text.Append('\\', slashes * 2).Append('"');
            return text.ToString();
        }
    }
}
