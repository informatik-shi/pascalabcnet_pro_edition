using System.Text.Json;

namespace PascalABCNotebook;

internal sealed class NotebookCell
{
    public Guid Id { get; set; } = Guid.NewGuid();
    public string Code { get; set; } = "";
    public string Output { get; set; } = "";
    public string? ImageUrl { get; set; }
    public bool Succeeded { get; set; } = true;
}

internal sealed class NotebookDocument
{
    public Guid Id { get; set; } = Guid.NewGuid();
    public string Title { get; set; } = "Без названия";
    public string Language { get; set; } = "pascal";
    public DateTime UpdatedUtc { get; set; } = DateTime.UtcNow;
    public List<NotebookCell> Cells { get; set; } = [];
}

internal sealed record NotebookSummary(Guid Id, string Title, string Language,
    DateTime UpdatedUtc);

internal sealed class NotebookStore(string workDirectory)
{
    internal static readonly JsonSerializerOptions JsonOptions = new(JsonSerializerDefaults.Web)
    {
        WriteIndented = true
    };

    internal string DirectoryPath { get; } = Path.Combine(workDirectory, "Notebooks");

    internal NotebookDocument Create(string language)
    {
        if (language is not ("pascal" or "spython"))
            throw new ArgumentException("Язык должен быть pascal или spython.");
        var notebook = new NotebookDocument
        {
            Title = language == "pascal" ? "Новая тетрадка PascalABC.NET" : "Новая тетрадка SPython",
            Language = language,
            Cells = [new NotebookCell()]
        };
        Save(notebook);
        return notebook;
    }

    internal IReadOnlyList<NotebookSummary> List()
    {
        Directory.CreateDirectory(DirectoryPath);
        var notebooks = new List<NotebookSummary>();
        foreach (var path in Directory.EnumerateFiles(DirectoryPath, "*.json"))
        {
            try
            {
                var notebook = JsonSerializer.Deserialize<NotebookDocument>(File.ReadAllText(path), JsonOptions);
                if (notebook is not null)
                    notebooks.Add(new(notebook.Id, notebook.Title, notebook.Language,
                        notebook.UpdatedUtc));
            }
            catch (JsonException) { /* A damaged file does not hide other notebooks. */ }
        }
        return notebooks.OrderByDescending(n => n.UpdatedUtc).ToList();
    }

    internal NotebookDocument? Load(Guid id)
    {
        var path = NotebookPath(id);
        if (!File.Exists(path)) return null;
        return JsonSerializer.Deserialize<NotebookDocument>(File.ReadAllText(path), JsonOptions);
    }

    internal void Save(NotebookDocument notebook)
    {
        Validate(notebook);
        Directory.CreateDirectory(DirectoryPath);
        notebook.UpdatedUtc = DateTime.UtcNow;
        var path = NotebookPath(notebook.Id);
        var temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
        File.WriteAllText(temporary, JsonSerializer.Serialize(notebook, JsonOptions));
        File.Move(temporary, path, true);
    }

    internal string RunDirectory(Guid notebookId, Guid runId)
    {
        var path = Path.Combine(DirectoryPath, notebookId.ToString("N"), "runs",
            runId.ToString("N"));
        Directory.CreateDirectory(path);
        return path;
    }

    internal string ImagePath(Guid notebookId, Guid runId) =>
        Path.Combine(DirectoryPath, notebookId.ToString("N"), "runs",
            runId.ToString("N"), "image.png");

    private string NotebookPath(Guid id) => Path.Combine(DirectoryPath, id.ToString("N") + ".json");

    internal static void Validate(NotebookDocument notebook)
    {
        if (notebook.Language is not ("pascal" or "spython"))
            throw new ArgumentException("Неизвестный язык тетрадки.");
        if (notebook.Id == Guid.Empty) throw new ArgumentException("Некорректный ID тетрадки.");
        notebook.Title = notebook.Title?.Trim() ?? "";
        if (notebook.Title.Length is < 1 or > 120)
            throw new ArgumentException("Название должно содержать от 1 до 120 символов.");
        if (notebook.Cells is null || notebook.Cells.Count is < 1 or > 100)
            throw new ArgumentException("В тетрадке должно быть от 1 до 100 ячеек.");
        if (notebook.Cells.Any(c => c is null || c.Code is null || c.Code.Length > 50_000))
            throw new ArgumentException("Размер ячейки не должен превышать 50 000 символов.");
        if (notebook.Cells.Sum(c => c.Code.Length) > 500_000)
            throw new ArgumentException("Размер тетрадки не должен превышать 500 000 символов.");
        if (notebook.Cells.Any(c => c.Output is null || c.Output.Length > 2_000_000))
            throw new ArgumentException("Слишком большой вывод ячейки.");
    }
}
