using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading.Tasks;

namespace VisualPascalABC.Classroom
{
    internal sealed class LocalTestResult
    {
        public int Passed { get; set; }
        public int Total { get; set; }
        public string Message { get; set; }
    }

    internal static class LocalTestRunner
    {
        private const int MaxOutput = 65536;

        public static LocalTestResult Run(string sourceFile, IList<ClassroomTest> tests,
                                          string compilerPath = null)
        {
            var result = new LocalTestResult { Total = tests.Count };
            if (tests.Count == 0)
            {
                result.Message = "Открытых тестов нет";
                return result;
            }
            var compiler = compilerPath ?? Path.Combine(AppDomain.CurrentDomain.BaseDirectory,
                                                         "pabcnetcclear.exe");
            if (!File.Exists(compiler))
                throw new FileNotFoundException("Не найден локальный компилятор", compiler);
            var directory = Path.Combine(Path.GetTempPath(), "abc-pro-classroom-" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(directory);
            try
            {
                var executable = Path.Combine(directory, "solution.exe");
                var sourceDirectory = Path.GetDirectoryName(sourceFile);
                var compile = Execute(compiler,
                    "/Output:" + Quote(executable) + " /SearchDir:" + Quote(sourceDirectory) + " " + Quote(sourceFile),
                    directory, "", 30000);
                if (compile.ExitCode != 0 || !File.Exists(executable))
                {
                    result.Message = "Ошибка локальной компиляции: " + compile.Output;
                    return result;
                }
                for (var i = 0; i < tests.Count; i++)
                {
                    var run = Execute(executable, "", sourceDirectory, tests[i].input ?? "", 5000);
                    if (run.ExitCode != 0 || Normalize(run.Output) != Normalize(tests[i].expected ?? ""))
                    {
                        result.Message = "Не пройден открытый тест " + (i + 1) +
                                         " (код " + run.ExitCode + ", вывод: " +
                                         run.Output.Substring(0, Math.Min(run.Output.Length, 200)) + ")";
                        return result;
                    }
                    result.Passed++;
                }
                result.Message = "Открытые тесты пройдены";
                return result;
            }
            finally
            {
                try { Directory.Delete(directory, true); }
                catch (IOException) { }
                catch (UnauthorizedAccessException) { }
            }
        }

        private static string Normalize(string text)
        {
            return text.Replace("\r\n", "\n").Replace("\r", "\n").TrimEnd('\n');
        }

        private static string Quote(string value)
        {
            return "\"" + value.Replace("\"", "\\\"") + "\"";
        }

        private sealed class ProcessResult
        {
            public int ExitCode;
            public string Output;
        }

        private static ProcessResult Execute(string file, string arguments, string workingDirectory,
                                             string input, int timeoutMs)
        {
            // Process.StandardInput inherits Console.InputEncoding on .NET Framework.
            // A BOM becomes part of the first Pascal Readln value, so select UTF-8 without it.
            Console.InputEncoding = new UTF8Encoding(false);
            var start = new ProcessStartInfo(file, arguments)
            {
                WorkingDirectory = workingDirectory,
                UseShellExecute = false,
                CreateNoWindow = true,
                RedirectStandardInput = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                StandardOutputEncoding = Encoding.UTF8,
                StandardErrorEncoding = Encoding.UTF8
            };
            using (var process = Process.Start(start))
            {
                var writer = Task.Factory.StartNew(() =>
                {
                    try
                    {
                        var normalized = input.Replace("\r\n", "\n").Replace("\r", "\n")
                                              .Replace("\n", "\r\n");
                        var bytes = new UTF8Encoding(false).GetBytes(normalized);
                        process.StandardInput.BaseStream.Write(bytes, 0, bytes.Length);
                        process.StandardInput.BaseStream.Flush();
                    }
                    catch (IOException) { }
                    finally { try { process.StandardInput.BaseStream.Close(); } catch (IOException) { } }
                }, TaskCreationOptions.LongRunning);
                var stdout = Task.Factory.StartNew(() => ReadBounded(process.StandardOutput, process),
                                                   TaskCreationOptions.LongRunning);
                var stderr = Task.Factory.StartNew(() => ReadBounded(process.StandardError, process),
                                                   TaskCreationOptions.LongRunning);
                if (!process.WaitForExit(timeoutMs))
                {
                    try { process.Kill(); } catch (InvalidOperationException) { }
                    process.WaitForExit();
                    return new ProcessResult { ExitCode = 124, Output = "Превышен лимит времени" };
                }
                if (!Task.WaitAll(new Task[] { stdout, stderr, writer }, 2000))
                    return new ProcessResult { ExitCode = 124, Output = "Не удалось получить вывод" };
                var output = stdout.Result + stderr.Result;
                return new ProcessResult { ExitCode = process.ExitCode, Output = output };
            }
        }

        private static string ReadBounded(StreamReader reader, Process process)
        {
            var output = new StringBuilder();
            var buffer = new char[2048];
            int count;
            while ((count = reader.Read(buffer, 0, buffer.Length)) > 0)
            {
                output.Append(buffer, 0, count);
                if (output.Length > MaxOutput)
                {
                    try { process.Kill(); } catch (InvalidOperationException) { }
                    return "Превышен лимит объёма вывода";
                }
            }
            return output.ToString();
        }
    }
}
