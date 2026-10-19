using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Text;
using Microsoft.Win32;

namespace ABCPro.Diagnostics
{
    // BCL only: safe before loading IDE/plugins. No arguments, environment dumps,
    // credentials or user document contents are collected.
    internal static class DiagnosticLog
    {
        private static readonly object Sync = new object();
        private static readonly StringBuilder Pending = new StringBuilder();
        private static readonly StringBuilder History = new StringBuilder();
        private static string[] directories;
        private static string name;
        internal static string FilePath { get; private set; }
        internal static string Stage { get; private set; }
        internal static bool Failed { get; private set; }

        internal static void Initialize(string component)
        {
            try {
                var session = Environment.GetEnvironmentVariable("PABC_DIAGNOSTIC_SESSION");
                Guid parsed;
                if (!Guid.TryParseExact(session, "N", out parsed)) session = Guid.NewGuid().ToString("N");
                Environment.SetEnvironmentVariable("PABC_DIAGNOSTIC_SESSION", session);
                name = component + "-" + session + ".log";
                directories = new[] {
                    SafePath(() => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "abc.pro", "Logs")),
                    SafePath(() => Path.Combine(Path.GetTempPath(), "abc.pro", "Logs")),
                    SafePath(() => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "abc.pro", "Logs")) };
                SetStage("bootstrap " + component);
                Write("Session: " + session + "; UTC: " + DateTime.UtcNow.ToString("o"));
                Write("Build: " + Assembly.GetExecutingAssembly().GetName().Version + "; " +
                    Assembly.GetExecutingAssembly().GetCustomAttribute<AssemblyInformationalVersionAttribute>()?.InformationalVersion);
                Write("Executable directory: " + AppDomain.CurrentDomain.BaseDirectory);
                Write("OS: " + Environment.OSVersion + "; OS64=" + Environment.Is64BitOperatingSystem +
                    "; Process64=" + Environment.Is64BitProcess + "; CLR=" + Environment.Version);
                try {
                    using (var key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full"))
                        Write("Required .NET Framework >=4.7.2; installed Release=" + (key == null ? "unknown" : Convert.ToString(key.GetValue("Release"))));
                } catch (Exception e) { Write("Framework detection unavailable: " + e); }
                Write("Launcher report: " + Environment.GetEnvironmentVariable("PABC_DIAGNOSTIC_LAUNCH_LOG"));
            } catch { /* Even fallback selection must never break startup. */ }
        }

        private static string SafePath(Func<string> getPath) { try { var p = getPath(); return Path.IsPathRooted(p) ? p : null; } catch { return null; } }
        internal static void SetStage(string stage) { Stage = stage; Write("Stage: " + stage); }
        internal static void Write(string message)
        {
            try {
                lock (Sync) {
                    var line = DateTime.UtcNow.ToString("o") + " [" + Stage + "] " + message + Environment.NewLine;
                    Pending.Append(line);
                    History.Append(line);
                    if (History.Length > 262144) History.Remove(0, History.Length - 131072);
                    if (directories == null) return;
                    foreach (var directory in directories) {
                        if (string.IsNullOrEmpty(directory)) continue;
                        try {
                            var target = Path.Combine(directory, name);
                            Directory.CreateDirectory(directory);
                            File.AppendAllText(target, target == FilePath ? Pending.ToString() : History.ToString(), new UTF8Encoding(false));
                            FilePath = target;
                            Pending.Clear();
                            return;
                        } catch (Exception e) {
                            var unavailable = "Log destination unavailable: " + directory + "; " + e.GetType().FullName + ": " + e.Message + Environment.NewLine;
                            Pending.Append(unavailable);
                            History.Append(unavailable);
                        }
                    }
                    if (Pending.Length > 262144) Pending.Remove(0, Pending.Length - 131072);
                    FilePath = null;
                }
            } catch { }
        }
        internal static void Failure(Exception error)
        {
            Failed = true;
            Write("FAILURE (observed): " + error);
            var types = error as ReflectionTypeLoadException;
            if (types != null && types.LoaderExceptions != null)
                foreach (var loader in types.LoaderExceptions) Write("Loader exception: " + loader);
        }
        internal static string RelatedReport(string component)
        {
            try {
                foreach (var directory in directories) {
                    if (string.IsNullOrEmpty(directory)) continue;
                    var path = Path.Combine(directory, component + "-" + Environment.GetEnvironmentVariable("PABC_DIAGNOSTIC_SESSION") + ".log");
                    if (File.Exists(path)) return path;
                }
            } catch { }
            return null;
        }
        internal static void ProbeDirectory(string label, string path)
        {
            try {
                Directory.CreateDirectory(path);
                var probe = Path.Combine(path, ".abc-pro-probe-" + Guid.NewGuid().ToString("N"));
                using (new FileStream(probe, FileMode.CreateNew, FileAccess.Write, FileShare.None, 1, FileOptions.DeleteOnClose)) { }
                Write(label + ": writable; " + path);
            } catch (Exception e) { Write(label + ": unavailable; " + path + "; " + e); throw; }
        }
        internal static void DescribeRuntime(string root)
        {
            foreach (var relative in new[] { @"dotnet\dotnet.exe", @"dotnet\host\fxr", @"dotnet\shared\Microsoft.NETCore.App", @"dotnet\shared\Microsoft.WindowsDesktop.App", @"bin\PascalABCNET.exe", @"bin\PascalABCNET.exe.config", @"bin\Compiler.dll", @"python\python.exe" }) {
                try {
                    var path = Path.Combine(root, relative);
                    Write(relative + ": " + (File.Exists(path) ? "present; version=" + FileVersionInfo.GetVersionInfo(path).FileVersion :
                        Directory.Exists(path) ? string.Join(", ", Array.ConvertAll(Directory.GetDirectories(path), Path.GetFileName)) : "missing"));
                } catch (Exception e) { Write(relative + ": inspection failed; " + e); }
            }
        }
    }
}
