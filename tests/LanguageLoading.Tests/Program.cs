using System;
using System.IO;
using System.Reflection;
using ABCPro.Diagnostics;
using Languages.Facade;
using Languages.Integration;

class Program
{
    static int Main(string[] args)
    {
        try { Run(args[0]); return 0; }
        catch (Exception e) { Console.Error.WriteLine(e); return 1; }
    }
    static void Run(string mode)
    {
        var provider = LanguageProvider.Instance;
        try { var invalid = provider.MainLanguage; throw new Exception("Empty list was accepted"); }
        catch (InvalidOperationException e) { Check(e.Message.Contains("PascalABCLanguageInfo.dll"), "empty list explains missing main language"); }
        int errors = 0;
        Exception detail = null;
        LanguageIntegrator.LanguageLoadFailed += (path, error) => { errors++; detail = error; };
        Exception failure = null;
        try { LanguageIntegrator.LoadAllLanguages(); } catch (Exception e) { failure = e; }
        if (mode == "normal") {
            Check(failure == null && errors == 0 && provider.Languages.Count == 1, "normal main language loads");
            Check(provider.MainLanguage.Name == "Fixture Pascal", "unrelated Language class is ignored");
        } else if (mode == "missing") {
            Check(failure is FileNotFoundException && provider.Languages.Count == 0, "missing main module fails before optional language");
        } else if (mode == "optional") {
            Check(failure == null && provider.Languages.Count == 1 && errors == 1, "optional failure keeps valid Pascal main language");
            Check(detail.ToString().Contains("BadImageFormatException"), "optional failure preserves original exception");
        } else if (mode == "corrupt") {
            Check(failure is InvalidOperationException && errors == 1 && provider.Languages.Count == 0, "main load failure stops startup");
            Check(failure.InnerException is BadImageFormatException && failure.ToString().Contains("PascalABCLanguageInfo.dll"), "corrupt main module retains cause and filename");
        } else if (mode == "blocked") {
            Check(failure is InvalidOperationException && errors == 1 && provider.Languages.Count == 0, "internet-marked main module reports original failure");
            Check(failure.ToString().Contains("80131515") && failure.Message.Contains("Разблокировать"), "zone restriction is explained without enabling remote trust");
        } else throw new Exception("Unknown test mode");
        DiagnosticLog.Initialize("language-test");
        var loader = new ReflectionTypeLoadException(new Type[] { null }, new Exception[] { new FileNotFoundException("NESTED_LOADER_MARKER") });
        DiagnosticLog.WriteException("Language load: ", new InvalidOperationException("wrapped", loader));
        Check(File.ReadAllText(DiagnosticLog.FilePath).Contains("NESTED_LOADER_MARKER"), "nested LoaderExceptions reach report");
        Console.WriteLine("PASS: language loading " + mode);
    }
    static void Check(bool condition, string message) { if (!condition) throw new Exception("FAILED: " + message); Console.WriteLine("PASS: " + message); }
}
