using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Threading;
using System.Windows.Forms;
using ABCPro.Diagnostics;

// Exercise production handlers/launcher without opening failure dialogs.
namespace ABCPro.Diagnostics
{
    internal static class FailureDialog
    {
        internal static void Show(Exception error) { DiagnosticLog.Write("TEST failure dialog: " + error.Message); }
    }
}
namespace VisualPascalABC
{
    internal static class VisualPascalABCProgram
    {
        internal static void Run(string[] args)
        {
            DiagnosticLog.SetStage("test " + args[0]);
            if (args[0] == "loader") { UseMissingDependency(); return; }
            if (args[0] == "startup") throw new TypeInitializationException("Fixture", new IOException("Inner diagnostic marker"));
            if (args[0] == "thread") {
                new Thread(() => { throw new InvalidOperationException("Background diagnostic marker"); }).Start();
                Thread.Sleep(10000);
                return;
            }
            using (var form = new Form())
            using (var timer = new System.Windows.Forms.Timer { Interval = 100 }) {
                timer.Tick += (s, e) => { timer.Stop(); if (args[0] == "ui") throw new InvalidOperationException("UI diagnostic marker"); form.Close(); };
                timer.Start();
                Application.Run(form);
            }
        }
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining)]
        private static void UseMissingDependency() { Console.WriteLine(MissingDependency.Probe.GetValue()); }
    }
}
namespace DiagnosticTests
{
    internal static class Driver
    {
        [System.Runtime.InteropServices.DllImport("kernel32.dll")]
        private static extern uint SetErrorMode(uint mode);
        [STAThread]
        private static int Main(string[] args)
        {
            SetErrorMode(3); // Prevent Windows crash UI for intentional child failures.
            if (args.Length > 0 && args[0].StartsWith("--fixture-exit=")) return int.Parse(args[0].Substring(15));
            if (args.Length > 0 && args[0] == "--bootstrap") {
                typeof(VisualPascalABC.DiagnosticBootstrap).GetMethod("Main", BindingFlags.NonPublic | BindingFlags.Static)
                    .Invoke(null, new object[] { new[] { args[1] } });
                return Environment.ExitCode;
            }
            if (args.Length > 0 && args[0] == "--launcher")
                return (int)typeof(ABCProLauncher.Program).GetMethod("Main", BindingFlags.NonPublic | BindingFlags.Static)
                    .Invoke(null, new object[] { new[] { "--portable", "--fixture-exit=" + args[1] } });
            var root = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "cases-" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(root);
            var blocked = Path.Combine(root, "blocked");
            File.WriteAllText(blocked, "file blocks directory creation");
            DiagnosticLog.Initialize("test");
            var dirs = typeof(DiagnosticLog).GetField("directories", BindingFlags.NonPublic | BindingFlags.Static);
            dirs.SetValue(null, new[] { blocked, Path.Combine(root, "fallback") });
            DiagnosticLog.SetStage("fallback write test");
            var report = DiagnosticLog.FilePath;
            Check(report != null && report.Contains("fallback"), "fallback destination");
            try { throw new IOException("Outer marker", new InvalidOperationException("Inner marker")); }
            catch (Exception e) { DiagnosticLog.Failure(e); }
            var text = File.ReadAllText(report);
            Check(text.Contains("Outer marker") && text.Contains("Inner marker") && text.Contains("Driver.Main"), "exception chain and stack");
            Check(text.Contains("OS:") && text.Contains("Build:") && text.Contains("fallback write test"), "metadata survives fallback");
            dirs.SetValue(null, new[] { blocked });
            DiagnosticLog.Write("all destinations blocked");
            Check(DiagnosticLog.FilePath == null, "all unavailable does not throw or claim saved report");
            dirs.SetValue(null, new[] { Path.Combine(root, "recovery") });
            DiagnosticLog.Write("recovered");
            Check(File.ReadAllText(DiagnosticLog.FilePath).Contains("all destinations blocked"), "pending messages recovered");

            var self = Assembly.GetExecutingAssembly().Location;
            foreach (var mode in new[] { "startup", "loader", "ui", "thread", "normal" }) {
                var session = Guid.NewGuid().ToString("N");
                Environment.SetEnvironmentVariable("PABC_DIAGNOSTIC_SESSION", session);
                var executable = self;
                if (mode == "loader") {
                    var missingDirectory = Path.Combine(root, "missing-dependency");
                    Directory.CreateDirectory(missingDirectory);
                    executable = Path.Combine(missingDirectory, Path.GetFileName(self));
                    File.Copy(self, executable);
                }
                var start = new ProcessStartInfo(executable, "--bootstrap " + mode) { UseShellExecute = false, CreateNoWindow = true };
                using (var child = Process.Start(start)) {
                    Check(child.WaitForExit(20000), "handler finished " + mode);
                    Check(mode == "normal" ? child.ExitCode == 0 : child.ExitCode != 0, "handler exit status " + mode);
                }
                var log = FindLog("ide-" + session + ".log");
                Check(log != null, "handler report " + mode);
                var contents = File.ReadAllText(log);
                Check(mode == "normal" ? !contents.Contains("FAILURE") : contents.Contains("FAILURE") &&
                    (mode == "loader" ? contents.Contains("FileNotFoundException") && contents.Contains("MissingDependency") : contents.Contains("diagnostic marker")), "handler evidence " + mode);
            }
            // The production launcher monitors a stand-in process, including quick clean exit.
            var bin = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "bin");
            Directory.CreateDirectory(bin);
            File.Copy(self, Path.Combine(bin, "PascalABCNET.exe"), true);
            foreach (var relative in new[] { @"dotnet\dotnet.exe", @"python\python.exe", @"notebook\PascalABCNotebook.dll" }) {
                var path = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, relative);
                Directory.CreateDirectory(Path.GetDirectoryName(path)); File.WriteAllText(path, "fixture");
            }
            foreach (var code in new[] { 0, 17 }) {
                var session = Guid.NewGuid().ToString("N");
                Environment.SetEnvironmentVariable("PABC_DIAGNOSTIC_SESSION", session);
                using (var child = Process.Start(new ProcessStartInfo(self, "--launcher " + code) { UseShellExecute = false })) {
                    Check(child.WaitForExit(20000), "launcher finished");
                    Check(child.ExitCode == (code == 0 ? 0 : 1), "launcher observed exit " + code);
                }
                var contents = File.ReadAllText(FindLog("launcher-" + session + ".log"));
                Check(contents.Contains("IDE exit code=" + code), "exit evidence " + code);
                Check(contents.Contains("FAILURE") == (code != 0), "no false crash on quick close " + code);
            }
            Console.WriteLine("All startup diagnostic checks passed.");
            return 0;
        }
        private static string FindLog(string name)
        {
            foreach (var directory in new[] { Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), Path.GetTempPath(), Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) }) {
                var path = Path.Combine(directory, "abc.pro", "Logs", name);
                if (File.Exists(path)) return path;
            }
            return null;
        }
        private static void Check(bool pass, string name) { if (!pass) throw new Exception("FAILED: " + name); Console.WriteLine("PASS: " + name); }
    }
}
