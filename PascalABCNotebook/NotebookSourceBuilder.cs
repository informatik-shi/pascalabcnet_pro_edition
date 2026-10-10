using System.Text;
using System.Text.RegularExpressions;

namespace PascalABCNotebook;

internal enum GraphicsBackend { None, GraphABC, GraphWPF, Graph3D, PlotML, WPF, Matplotlib }

internal sealed record GeneratedSource(string Text, string Extension, string? StartMarker,
    string? EndMarker, GraphicsBackend Graphics);

internal static class NotebookSourceBuilder
{
    private static readonly Regex PascalUses = new(@"(?im)^\s*uses\s+([^;]+);",
        RegexOptions.Compiled);
    private static readonly Regex FullProgramEnd = new(@"\bend\s*\.\s*$",
        RegexOptions.IgnoreCase | RegexOptions.Singleline | RegexOptions.Compiled);
    private static readonly Regex GlobalDeclaration = new(@"^\s*(?:type|const|procedure|function)\b",
        RegexOptions.IgnoreCase | RegexOptions.Compiled);

    internal static GeneratedSource Build(NotebookDocument notebook, int index, string imagePath)
    {
        if (index < 0 || index >= notebook.Cells.Count)
            throw new ArgumentOutOfRangeException(nameof(index));

        var activeCode = notebook.Cells[index].Code;
        var allCode = string.Join("\n", notebook.Cells.Take(index + 1).Select(c => c.Code));
        var completeProgram = notebook.Language == "pascal"
            ? FullProgramEnd.Match(activeCode) : Match.Empty;
        var graphics = DetectGraphics(completeProgram.Success ? activeCode : allCode,
            notebook.Language);
        var imageLiteral = imagePath.Replace('\\', '/').Replace("'", "''");
        if (notebook.Language == "spython")
        {
            var marker = Guid.NewGuid().ToString("N");
            var start = "__PABC_NOTEBOOK_START_" + marker + "__";
            var end = "__PABC_NOTEBOOK_END_" + marker + "__";
            var source = new StringBuilder();
            if (graphics == GraphicsBackend.GraphABC)
                source.AppendLine("from GraphABC import *\nSetConsoleIO()");
            else if (graphics == GraphicsBackend.GraphWPF &&
                     !Regex.IsMatch(allCode, @"(?im)^\s*(?:from|import)\s+GraphWPF\b"))
                source.AppendLine("from GraphWPF import *");
            for (var i = 0; i <= index; i++)
            {
                if (i == index) source.AppendLine($"print('{start}')");
                source.AppendLine(notebook.Cells[i].Code);
                if (i == index) source.AppendLine($"print('{end}')");
            }
            source.AppendLine(SPythonCapture(graphics, imageLiteral, allCode));
            return new(source.ToString(), ".pys", start, end, graphics);
        }

        if (notebook.Language != "pascal")
            throw new ArgumentException("Unknown notebook language.");

        // A pasted, complete Pascal program is useful as a standalone cell.
        if (completeProgram.Success)
        {
            var capture = PascalCapture(graphics, imageLiteral);
            var text = capture.Length == 0 ? activeCode :
                activeCode.Insert(completeProgram.Index, ";\n" + capture + "\n");
            return new(text, ".pas", null, null, graphics);
        }

        var imports = new List<string>();
        var declarations = new List<string>();
        var statements = new List<string>();
        var token = Guid.NewGuid().ToString("N");
        var beginMarker = "__PABC_NOTEBOOK_START_" + token + "__";
        var endMarker = "__PABC_NOTEBOOK_END_" + token + "__";
        for (var i = 0; i <= index; i++)
        {
            var code = PascalUses.Replace(notebook.Cells[i].Code, match =>
            {
                foreach (var item in match.Groups[1].Value.Split(','))
                {
                    var name = item.Trim();
                    if (name.Length > 0 && !imports.Contains(name, StringComparer.OrdinalIgnoreCase))
                        imports.Add(name);
                }
                return "";
            }).Trim();
            if (code.Length == 0) continue;
            if (GlobalDeclaration.IsMatch(code))
            {
                declarations.Add(code);
                continue;
            }
            if (i == index)
                statements.Add($"System.Console.WriteLine('{beginMarker}');");
            statements.Add(EnsureSemicolon(code));
            if (i == index)
                statements.Add($"System.Console.WriteLine('{endMarker}');");
        }

        var backendUnit = graphics switch
        {
            GraphicsBackend.GraphABC => "GraphABC",
            GraphicsBackend.GraphWPF => "GraphWPF",
            GraphicsBackend.Graph3D => "Graph3D",
            GraphicsBackend.PlotML => "PlotML",
            GraphicsBackend.WPF => "WPF",
            _ => null
        };
        if (backendUnit is not null &&
            !imports.Contains(backendUnit, StringComparer.OrdinalIgnoreCase))
            imports.Add(backendUnit);

        var pascal = new StringBuilder();
        if (imports.Count > 0) pascal.AppendLine("uses " + string.Join(", ", imports) + ";");
        foreach (var declaration in declarations) pascal.AppendLine(declaration);
        pascal.AppendLine("begin");
        if (graphics == GraphicsBackend.GraphABC)
            pascal.AppendLine("GraphABC.SetConsoleIO;");
        foreach (var statement in statements) pascal.AppendLine(statement);
        pascal.AppendLine(PascalCapture(graphics, imageLiteral));
        pascal.AppendLine("end.");
        return new(pascal.ToString(), ".pas", beginMarker, endMarker, graphics);
    }

    private static string EnsureSemicolon(string code) =>
        code.TrimEnd().EndsWith(';') ? code : code + ";";

    private static GraphicsBackend DetectGraphics(string code, string language)
    {
        IEnumerable<string> modules = language == "pascal"
            ? PascalUses.Matches(code).Cast<Match>()
                .SelectMany(m => m.Groups[1].Value.Split(',')).Select(s => s.Trim())
            : Regex.Matches(code, @"(?im)^\s*(?:from|import)\s+([A-Za-z_][A-Za-z_0-9]*)")
                .Cast<Match>().Select(m => m.Groups[1].Value);
        var names = modules.ToHashSet(StringComparer.OrdinalIgnoreCase);
        if (language == "spython" &&
            Regex.IsMatch(code, @"(?im)^\s*(?:import\s+matplotlib\.pyplot\b|from\s+matplotlib\.pyplot\s+import\b|from\s+matplotlib\s+import\s+pyplot\b)"))
            return GraphicsBackend.Matplotlib;
        if (names.Contains("PlotML")) return GraphicsBackend.PlotML;
        if (names.Contains("Graph3D")) return GraphicsBackend.Graph3D;
        if (names.Overlaps(["GraphWPF", "PlotWPF", "WPFObjects", "TurtleWPF", "Turtle"]))
            return GraphicsBackend.GraphWPF;
        if (names.Overlaps(["GraphABC", "ABCObjects", "ABCButtons", "ABCSprites",
                "ABCHouse", "TurtleABC", "Drawman", "DrawManField"]))
            return GraphicsBackend.GraphABC;
        if (names.Contains("WPF")) return GraphicsBackend.WPF;
        return GraphicsBackend.None;
    }

    private static string PascalCapture(GraphicsBackend graphics, string imagePath) => graphics switch
    {
        GraphicsBackend.GraphABC =>
            $"GraphABC.SaveWindow('{imagePath}');\nGraphABC.CloseWindow;",
        GraphicsBackend.GraphWPF =>
            $"GraphWPF.GraphWindow.Save('{imagePath}');\nGraphWPF.Window.Close;",
        GraphicsBackend.Graph3D =>
            $"Graph3D.View3D.Save('{imagePath}');\nGraph3D.Window.Close;",
        GraphicsBackend.PlotML =>
            $"PlotML.Plot.Save('{imagePath}');\nHalt;",
        GraphicsBackend.WPF =>
            $"WPF.SaveWindow('{imagePath}');",
        _ => ""
    };

    private static string SPythonCapture(GraphicsBackend graphics, string imagePath, string code)
    {
        if (graphics == GraphicsBackend.None) return "";
        if (graphics == GraphicsBackend.Matplotlib)
        {
            var pyplotImport = Regex.Match(code,
                @"(?im)^\s*import\s+matplotlib\.pyplot\s+as\s+([A-Za-z_][A-Za-z_0-9]*)");
            if (pyplotImport.Success)
                return $"{pyplotImport.Groups[1].Value}.savefig('{imagePath}')";
            pyplotImport = Regex.Match(code,
                @"(?im)^\s*from\s+matplotlib\s+import\s+pyplot(?:\s+as\s+([A-Za-z_][A-Za-z_0-9]*))?");
            if (pyplotImport.Success)
                return $"{(pyplotImport.Groups[1].Success ? pyplotImport.Groups[1].Value : "pyplot")}.savefig('{imagePath}')";
            return $"from matplotlib.pyplot import savefig\nsavefig('{imagePath}')";
        }
        var module = graphics.ToString();
        var importedNamespace = Regex.IsMatch(code,
            @"(?im)^\s*import\s+" + Regex.Escape(module) + @"\b");
        var prefix = importedNamespace ? module + "." : "";
        return graphics switch
        {
            GraphicsBackend.GraphABC =>
                $"{prefix}SaveWindow('{imagePath}')\n{prefix}CloseWindow()",
            GraphicsBackend.GraphWPF =>
                $"{prefix}GraphWindow.Save('{imagePath}')\n{prefix}Window.Close()",
            GraphicsBackend.Graph3D =>
                $"{prefix}View3D.Save('{imagePath}')\n{prefix}Window.Close()",
            GraphicsBackend.PlotML =>
                $"{prefix}Plot.Save('{imagePath}')",
            GraphicsBackend.WPF =>
                $"{prefix}SaveWindow('{imagePath}')",
            _ => ""
        };
    }
}
