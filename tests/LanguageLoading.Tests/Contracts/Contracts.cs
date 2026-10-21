using System;
namespace Languages.Facade
{
    public interface ILanguage { string Name { get; } string[] FilesExtensions { get; } }
}
namespace PascalABCCompiler
{
    public static class StringConstants { public const string pascalLanguageDllName = "PascalABCLanguageInfo.dll"; }
}
namespace PascalABCCompiler.Errors
{
    public class ParserBadFileExtension : Exception { public ParserBadFileExtension(string name) : base(name) { } }
}
