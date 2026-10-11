using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace ABCProLauncher
{
    internal static class Program
    {
        [STAThread]
        private static int Main(string[] args)
        {
            try
            {
                var root = AppDomain.CurrentDomain.BaseDirectory.TrimEnd(
                    Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
                var bin = Path.Combine(root, "bin");
                var ide = Path.Combine(bin, "PascalABCNET.exe");
                var dotnet = Path.Combine(root, "dotnet");
                var python = Path.Combine(root, "python", "python.exe");
                var notebook = Path.Combine(root, "notebook", "PascalABCNotebook.dll");
                foreach (var required in new[] { ide, Path.Combine(dotnet, "dotnet.exe"), python, notebook })
                    if (!File.Exists(required))
                        throw new FileNotFoundException("Неполная установка abc.pro", required);

                // Installer smoke tests can validate the package without starting the IDE.
                if (args.Length == 1 && args[0] == "--self-test")
                    return 0;

                var data = Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                    "abc.pro");
                var work = Path.Combine(data, "Work");
                Directory.CreateDirectory(work);
                Directory.CreateDirectory(Path.Combine(work, "Output"));
                Directory.CreateDirectory(Path.Combine(work, "Matplotlib"));
                CopyStarterDirectory(Path.Combine(root, "Templates", "Samples"),
                    Path.Combine(work, "Samples"));
                CopyStarterDirectory(Path.Combine(root, "Templates", "Files"),
                    Path.Combine(work, "Files"));
                File.WriteAllText(Path.Combine(bin, "pabcworknet.ini"), work,
                    new UTF8Encoding(false));

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
                Process.Start(start);
                return 0;
            }
            catch (Exception error)
            {
                MessageBox.Show(error.Message, "abc.pro", MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return 1;
            }
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
