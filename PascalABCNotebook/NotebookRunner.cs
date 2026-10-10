using System.Diagnostics;
using System.Text;

namespace PascalABCNotebook;

internal sealed record RunResult(bool Succeeded, string Output, string? ImageUrl,
    string Stage, int? ExitCode, long DurationMs);

internal sealed class NotebookRunner(string root, NotebookStore store)
{
    private readonly SemaphoreSlim executionGate = new(1, 1);
    private const int MaxOutputCharacters = 1_000_000;

    internal async Task<RunResult> RunAsync(NotebookDocument notebook, int index,
        CancellationToken cancellationToken)
    {
        await executionGate.WaitAsync(cancellationToken);
        try
        {
            return await RunCoreAsync(notebook, index, cancellationToken);
        }
        finally { executionGate.Release(); }
    }

    private async Task<RunResult> RunCoreAsync(NotebookDocument notebook, int index,
        CancellationToken cancellationToken)
    {
        var timer = Stopwatch.StartNew();
        var runId = Guid.NewGuid();
        var runDirectory = store.RunDirectory(notebook.Id, runId);
        var imagePath = store.ImagePath(notebook.Id, runId);
        var generated = NotebookSourceBuilder.Build(notebook, index, imagePath);
        var sourcePath = Path.Combine(runDirectory, "cell" + generated.Extension);
        await File.WriteAllTextAsync(sourcePath, generated.Text, new UTF8Encoding(false),
            cancellationToken);
        var compiler = Path.Combine(root, "bin", "pabcnetcclear.exe");
        if (!File.Exists(compiler))
            return new(false, "Не найден компилятор: " + compiler, null, "setup", null,
                timer.ElapsedMilliseconds);

        var compilation = await ExecuteAsync(compiler, runDirectory, [sourcePath],
            TimeSpan.FromSeconds(45), cancellationToken);
        if (compilation.TimedOut)
            return new(false, "Превышено время компиляции (45 секунд).", null,
                "compile", null, timer.ElapsedMilliseconds);
        var compileText = Merge(compilation.Stdout, compilation.Stderr);
        var executable = Path.ChangeExtension(sourcePath, ".exe");
        if (compilation.ExitCode != 0 || !File.Exists(executable))
            return new(false, compileText, null, "compile", compilation.ExitCode,
                timer.ElapsedMilliseconds);

        foreach (var dependency in Directory.EnumerateFiles(Path.Combine(root, "bin", "Lib"),
                     "*.dll"))
            File.Copy(dependency, Path.Combine(runDirectory, Path.GetFileName(dependency)), true);

        var execution = await ExecuteAsync(executable, runDirectory, [],
            TimeSpan.FromSeconds(60), cancellationToken,
            generated.Graphics != GraphicsBackend.None, root);
        var output = Merge(execution.Stdout, execution.Stderr);
        if (execution.TimedOut)
            output += "\nПрограмма остановлена: превышено время выполнения (60 секунд).";
        output = CurrentCellOutput(output, generated.StartMarker, generated.EndMarker);
        var imageUrl = File.Exists(imagePath)
            ? $"/api/notebooks/{notebook.Id:N}/images/{runId:N}.png"
            : null;
        // The graphical units end the process with Halt after saving their image.
        var succeeded = !execution.TimedOut &&
            (execution.ExitCode == 0 || (generated.Graphics != GraphicsBackend.None &&
                imageUrl is not null && execution.ExitCode == -1));
        if (!succeeded && output.Length == 0)
            output = $"Программа завершилась с кодом {execution.ExitCode}.";
        return new(succeeded, output, imageUrl, "run", execution.ExitCode,
            timer.ElapsedMilliseconds);
    }

    private static string CurrentCellOutput(string output, string? start, string? end)
    {
        if (start is null || end is null) return output.Trim();
        var first = output.IndexOf(start, StringComparison.Ordinal);
        if (first < 0) return output.Trim();
        var from = first + start.Length;
        var last = output.IndexOf(end, from, StringComparison.Ordinal);
        if (last < 0) return output[from..].Trim();
        var current = output[from..last].Trim();
        var after = output[(last + end.Length)..].Trim();
        return after.Length == 0 ? current : (current + "\n" + after).Trim();
    }

    private static string Merge(string stdout, string stderr) =>
        (stdout + (stderr.Length > 0 ? "\n" + stderr : "")).Trim();

    private static async Task<ProcessResult> ExecuteAsync(string fileName,
        string workingDirectory, IReadOnlyList<string> arguments, TimeSpan timeout,
        CancellationToken cancellationToken, bool inlineGraphics = false,
        string? packageRoot = null)
    {
        using var process = new Process();
        process.StartInfo = new ProcessStartInfo(fileName)
        {
            WorkingDirectory = workingDirectory,
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            StandardOutputEncoding = Encoding.UTF8,
            StandardErrorEncoding = Encoding.UTF8
        };
        if (inlineGraphics) process.StartInfo.Environment["PABC_NOTEBOOK_INLINE"] = "1";
        if (packageRoot is not null)
        {
            process.StartInfo.Environment["PABCNET_ROOT"] = packageRoot;
            var bundledPython = Path.Combine(packageRoot, "python", "python.exe");
            if (File.Exists(bundledPython))
                process.StartInfo.Environment["PABC_PYTHON_EXE"] = bundledPython;
            var matplotlibCache = Path.Combine(packageRoot, "Work", "Matplotlib");
            Directory.CreateDirectory(matplotlibCache);
            process.StartInfo.Environment["MPLCONFIGDIR"] = matplotlibCache;
        }
        foreach (var argument in arguments) process.StartInfo.ArgumentList.Add(argument);
        process.Start();
        var stdoutTask = ReadLimitedAsync(process.StandardOutput);
        var stderrTask = ReadLimitedAsync(process.StandardError);
        using var timeoutSource = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        timeoutSource.CancelAfter(timeout);
        var timedOut = false;
        try { await process.WaitForExitAsync(timeoutSource.Token); }
        catch (OperationCanceledException) when (!cancellationToken.IsCancellationRequested)
        {
            timedOut = true;
            process.Kill(entireProcessTree: true);
            await process.WaitForExitAsync(CancellationToken.None);
        }
        catch (OperationCanceledException)
        {
            if (!process.HasExited)
            {
                process.Kill(entireProcessTree: true);
                await process.WaitForExitAsync(CancellationToken.None);
            }
            throw;
        }
        var stdout = await stdoutTask;
        var stderr = await stderrTask;
        return new(process.ExitCode, stdout, stderr, timedOut);
    }

    private static async Task<string> ReadLimitedAsync(StreamReader reader)
    {
        var text = new StringBuilder();
        var buffer = new char[8192];
        int count;
        while ((count = await reader.ReadAsync(buffer)) != 0)
        {
            var available = MaxOutputCharacters - text.Length;
            if (available > 0) text.Append(buffer, 0, Math.Min(count, available));
        }
        if (text.Length == MaxOutputCharacters) text.Append("\n[Вывод сокращён]");
        return text.ToString();
    }

    private sealed record ProcessResult(int ExitCode, string Stdout, string Stderr,
        bool TimedOut);
}
