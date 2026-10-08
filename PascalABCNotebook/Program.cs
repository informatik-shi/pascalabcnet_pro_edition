using System.Collections.Concurrent;
using System.Diagnostics;
using System.Net;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace PascalABCNotebook;

internal sealed class NotebookServer
{
    private readonly string root;
    private readonly NotebookStore store;
    private readonly NotebookRunner runner;
    private readonly string sessionToken = Convert.ToHexString(RandomNumberGenerator.GetBytes(32));
    private readonly ConcurrentDictionary<Guid, SemaphoreSlim> documentLocks = new();
    private readonly string webRoot = Path.Combine(AppContext.BaseDirectory, "wwwroot");

    internal NotebookServer(string root, string workDirectory)
    {
        this.root = root;
        store = new(workDirectory);
        runner = new(root, store);
    }

    internal async Task RunAsync(int requestedPort, bool openBrowser)
    {
        var port = requestedPort == 0 ? FreePort() : requestedPort;
        var url = $"http://127.0.0.1:{port}/";
        using var listener = new HttpListener();
        listener.Prefixes.Add(url);
        listener.Start();
        Console.WriteLine("PascalABC.NET Notebook: " + url);
        Console.WriteLine("Данные: " + store.DirectoryPath);
        Console.WriteLine("Для остановки нажмите Ctrl+C.");
        if (openBrowser)
        {
            try { Process.Start(new ProcessStartInfo(url) { UseShellExecute = true }); }
            catch (Exception e) { Console.WriteLine("Откройте адрес вручную: " + e.Message); }
        }

        Console.CancelKeyPress += (_, e) =>
        {
            e.Cancel = true;
            listener.Stop();
        };
        while (listener.IsListening)
        {
            HttpListenerContext context;
            try { context = await listener.GetContextAsync(); }
            catch (HttpListenerException) when (!listener.IsListening) { break; }
            _ = Task.Run(() => HandleAsync(context));
        }
    }

    private async Task HandleAsync(HttpListenerContext context)
    {
        try
        {
            var request = context.Request;
            var path = request.Url?.AbsolutePath ?? "/";
            context.Response.Headers["Cache-Control"] = "no-store";
            context.Response.Headers["X-Content-Type-Options"] = "nosniff";
            context.Response.Headers["Content-Security-Policy"] =
                "default-src 'self'; img-src 'self' data:; style-src 'self'; script-src 'self'; " +
                "connect-src 'self'; object-src 'none'; base-uri 'none'";
            if (request.HttpMethod is "POST" or "PUT" &&
                request.Headers["X-Notebook-Token"] != sessionToken)
            {
                await JsonAsync(context, new { error = "Недействительный токен сессии." }, 403);
                return;
            }

            if (request.HttpMethod == "GET" && path == "/health")
                await JsonAsync(context, new { status = "ok" });
            else if (request.HttpMethod == "GET" && path == "/api/session")
                await JsonAsync(context, new { token = sessionToken });
            else if (request.HttpMethod == "GET" && path == "/api/notebooks")
                await JsonAsync(context, store.List());
            else if (request.HttpMethod == "POST" && path == "/api/notebooks")
            {
                var requestBody = await ReadJsonAsync<CreateRequest>(request);
                await JsonAsync(context, store.Create(requestBody.Language), 201);
            }
            else if (path.StartsWith("/api/notebooks/", StringComparison.Ordinal))
                await HandleNotebookAsync(context, path);
            else if (request.HttpMethod == "GET")
                await ServeStaticAsync(context, path);
            else
                await JsonAsync(context, new { error = "Маршрут не найден." }, 404);
        }
        catch (Exception error) when (error is ArgumentException or JsonException or InvalidDataException)
        {
            await JsonAsync(context, new { error = error.Message }, 400);
        }
        catch (Exception error)
        {
            Console.Error.WriteLine(error);
            try { await JsonAsync(context, new { error = "Внутренняя ошибка сервера." }, 500); }
            catch { /* The client may have disconnected. */ }
        }
        finally
        {
            try { context.Response.Close(); } catch { }
        }
    }

    private async Task HandleNotebookAsync(HttpListenerContext context, string path)
    {
        var request = context.Request;
        var parts = path.Trim('/').Split('/');
        if (parts.Length < 3 || !Guid.TryParse(parts[2], out var id))
        {
            await JsonAsync(context, new { error = "Тетрадка не найдена." }, 404);
            return;
        }
        if (parts.Length == 5 && parts[3] == "images" &&
            request.HttpMethod == "GET" && parts[4].EndsWith(".png", StringComparison.Ordinal) &&
            Guid.TryParse(parts[4][..^4], out var runId))
        {
            if (store.Load(id) is null)
            {
                await JsonAsync(context, new { error = "Тетрадка не найдена." }, 404);
                return;
            }
            var image = store.ImagePath(id, runId);
            if (!File.Exists(image))
                await JsonAsync(context, new { error = "Изображение не найдено." }, 404);
            else
                await BytesAsync(context, await File.ReadAllBytesAsync(image), "image/png");
            return;
        }
        if (parts.Length == 3 && request.HttpMethod == "GET")
        {
            var document = store.Load(id);
            await JsonAsync(context, document ?? (object)new { error = "Тетрадка не найдена." },
                document is null ? 404 : 200);
            return;
        }
        if ((parts.Length == 3 && request.HttpMethod == "PUT") ||
            (parts.Length == 4 && parts[3] == "run" && request.HttpMethod == "POST"))
        {
            var gate = documentLocks.GetOrAdd(id, _ => new SemaphoreSlim(1, 1));
            await gate.WaitAsync();
            try
            {
                var document = store.Load(id);
                if (document is null)
                {
                    await JsonAsync(context, new { error = "Тетрадка не найдена." }, 404);
                    return;
                }
                if (request.HttpMethod == "PUT")
                {
                    var incoming = await ReadJsonAsync<NotebookDocument>(request);
                    if (incoming.Id != id || incoming.Language != document.Language)
                        throw new ArgumentException("Нельзя менять ID или язык тетрадки.");
                    NotebookStore.Validate(incoming);
                    var previous = document.Cells.ToDictionary(c => c.Id);
                    foreach (var cell in incoming.Cells)
                    {
                        if (cell.Id == Guid.Empty) cell.Id = Guid.NewGuid();
                        if (previous.TryGetValue(cell.Id, out var oldCell))
                        {
                            cell.Output = oldCell.Output;
                            cell.ImageUrl = oldCell.ImageUrl;
                            cell.Succeeded = oldCell.Succeeded;
                        }
                        else
                        {
                            cell.Output = "";
                            cell.ImageUrl = null;
                            cell.Succeeded = true;
                        }
                    }
                    store.Save(incoming);
                    await JsonAsync(context, incoming);
                }
                else
                {
                    var body = await ReadJsonAsync<RunRequest>(request);
                    if (body.Index < 0 || body.Index >= document.Cells.Count)
                        throw new ArgumentException("Номер ячейки вне диапазона.");
                    var result = await runner.RunAsync(document, body.Index,
                        CancellationToken.None);
                    var cell = document.Cells[body.Index];
                    cell.Output = result.Output;
                    cell.ImageUrl = result.ImageUrl;
                    cell.Succeeded = result.Succeeded;
                    store.Save(document);
                    await JsonAsync(context, result);
                }
            }
            finally { gate.Release(); }
            return;
        }
        await JsonAsync(context, new { error = "Маршрут не найден." }, 404);
    }

    private async Task ServeStaticAsync(HttpListenerContext context, string path)
    {
        var file = path switch
        {
            "/" or "/index.html" => "index.html",
            "/app.js" => "app.js",
            "/style.css" => "style.css",
            _ => null
        };
        if (file is null)
        {
            await JsonAsync(context, new { error = "Файл не найден." }, 404);
            return;
        }
        var fullPath = Path.Combine(webRoot, file);
        var mime = file.EndsWith(".js") ? "text/javascript; charset=utf-8" :
            file.EndsWith(".css") ? "text/css; charset=utf-8" : "text/html; charset=utf-8";
        await BytesAsync(context, await File.ReadAllBytesAsync(fullPath), mime);
    }

    private static async Task<T> ReadJsonAsync<T>(HttpListenerRequest request)
    {
        if (request.ContentLength64 > 1_000_000)
            throw new InvalidDataException("Запрос слишком велик.");
        using var buffer = new MemoryStream();
        var chunk = new byte[8192];
        int count;
        while ((count = await request.InputStream.ReadAsync(chunk)) != 0)
        {
            buffer.Write(chunk, 0, count);
            if (buffer.Length > 1_000_000)
                throw new InvalidDataException("Запрос слишком велик.");
        }
        return JsonSerializer.Deserialize<T>(buffer.ToArray(), NotebookStore.JsonOptions)
            ?? throw new JsonException("Пустой JSON.");
    }

    private static Task JsonAsync(HttpListenerContext context, object value, int status = 200) =>
        BytesAsync(context, JsonSerializer.SerializeToUtf8Bytes(value, NotebookStore.JsonOptions),
            "application/json; charset=utf-8", status);

    private static async Task BytesAsync(HttpListenerContext context, byte[] bytes,
        string contentType, int status = 200)
    {
        context.Response.StatusCode = status;
        context.Response.ContentType = contentType;
        context.Response.ContentLength64 = bytes.Length;
        await context.Response.OutputStream.WriteAsync(bytes);
    }

    private static int FreePort()
    {
        var socket = new TcpListener(IPAddress.Loopback, 0);
        socket.Start();
        var port = ((IPEndPoint)socket.LocalEndpoint).Port;
        socket.Stop();
        return port;
    }

    private sealed record CreateRequest(string Language);
    private sealed record RunRequest(int Index);
}

internal static class Program
{
    private static async Task<int> Main(string[] args)
    {
        try
        {
            string? root = null;
            string? workDirectory = null;
            var port = 0;
            var browser = true;
            for (var i = 0; i < args.Length; i++)
            {
                switch (args[i])
                {
                    case "--root" when i + 1 < args.Length:
                        root = args[++i];
                        break;
                    case "--workdir" when i + 1 < args.Length:
                        workDirectory = args[++i];
                        break;
                    case "--port" when i + 1 < args.Length &&
                                       int.TryParse(args[i + 1], out var number) &&
                                       number is > 0 and <= 65535:
                        port = number;
                        i++;
                        break;
                    case "--no-browser":
                        browser = false;
                        break;
                    default:
                        throw new ArgumentException("Параметры: --root <папка> [--workdir <папка>] [--port <порт>] [--no-browser]");
                }
            }
            root = Path.GetFullPath(root ?? Directory.GetCurrentDirectory());
            if (!File.Exists(Path.Combine(root, "bin", "pabcnetcclear.exe")))
                throw new FileNotFoundException("Не найден bin\\pabcnetcclear.exe в папке " + root);
            workDirectory = Path.GetFullPath(workDirectory ?? Path.Combine(root, "Work"));
            await new NotebookServer(root, workDirectory).RunAsync(port, browser);
            return 0;
        }
        catch (Exception error)
        {
            Console.Error.WriteLine(error.Message);
            return 1;
        }
    }
}
