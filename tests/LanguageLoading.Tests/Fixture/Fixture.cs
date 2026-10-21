// A similarly named unrelated class must not hide the actual language.
public class UnrelatedLanguage { }
public class WorkingLanguage : Languages.Facade.ILanguage
{
    public string Name => "Fixture Pascal";
    public string[] FilesExtensions => new[] { ".pas" };
}
