using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Xml;
using ICSharpCode.TextEditor.Document;

namespace VisualPascalABC
{
    // Build dark definitions from the installed syntax files without changing their light versions.
    public sealed class DarkSyntaxModeProvider : ISyntaxModeFileProvider
    {
        private readonly ISyntaxModeFileProvider[] sources;
        private readonly List<SyntaxMode> modes = new List<SyntaxMode>();
        private readonly Dictionary<string, KeyValuePair<SyntaxMode, ISyntaxModeFileProvider>> originals =
            new Dictionary<string, KeyValuePair<SyntaxMode, ISyntaxModeFileProvider>>(StringComparer.OrdinalIgnoreCase);

        public DarkSyntaxModeProvider(params ISyntaxModeFileProvider[] sources)
        {
            this.sources = sources;
            UpdateSyntaxModeList();
        }

        public static string DarkName(string name)
        {
            return name + " Dark";
        }

        public ICollection<SyntaxMode> SyntaxModes
        {
            get { return modes; }
        }

        public void UpdateSyntaxModeList()
        {
            modes.Clear();
            originals.Clear();
            int index = 0;
            foreach (ISyntaxModeFileProvider provider in sources)
            {
                foreach (SyntaxMode source in provider.SyntaxModes)
                {
                    // SPython already has a hand-tuned dark palette and a separate light file.
                    if (source.Name == "Spython" || source.Name == "SpythonLight")
                        continue;
                    string name = DarkName(source.Name);
                    if (originals.ContainsKey(name))
                        modes.RemoveAll(mode => mode.Name == name);
                    originals[name] = new KeyValuePair<SyntaxMode, ISyntaxModeFileProvider>(source, provider);
                    modes.Add(new SyntaxMode(source.FileName, name, ".pabc-theme-dark-" + index++));
                }
            }
            modes.Add(new SyntaxMode("DefaultDark.xshd", DarkName("Default"), ".pabc-theme-dark-default"));
        }

        public XmlTextReader GetSyntaxModeFile(SyntaxMode syntaxMode)
        {
            XmlDocument document = new XmlDocument();
            KeyValuePair<SyntaxMode, ISyntaxModeFileProvider> original;
            if (originals.TryGetValue(syntaxMode.Name, out original))
            {
                using (XmlTextReader reader = original.Value.GetSyntaxModeFile(original.Key))
                    document.Load(reader);
            }
            else if (syntaxMode.Name == DarkName("Default"))
            {
                document.LoadXml("<SyntaxDefinition name='Default Dark' extensions='.pabc-theme-dark-default'><RuleSets><RuleSet /></RuleSets></SyntaxDefinition>");
            }
            else
                throw new ArgumentException("Unknown dark syntax mode: " + syntaxMode.Name);

            XmlElement root = document.DocumentElement;
            root.SetAttribute("name", syntaxMode.Name);
            root.SetAttribute("extensions", String.Join(";", syntaxMode.Extensions));
            if (root.HasAttribute("extends"))
                root.SetAttribute("extends", DarkName(root.GetAttribute("extends")));

            XmlElement previousEnvironment = root["Environment"];
            if (previousEnvironment != null)
                root.RemoveChild(previousEnvironment);
            foreach (XmlElement element in document.SelectNodes("//*[@color or @bgcolor or @reference]"))
            {
                if (element.HasAttribute("color"))
                    element.SetAttribute("color", DarkForeground(element.GetAttribute("color")));
                if (element.HasAttribute("bgcolor"))
                    element.SetAttribute("bgcolor", "#2D2D30");
                if (element.HasAttribute("reference") && originals.ContainsKey(DarkName(element.GetAttribute("reference"))))
                    element.SetAttribute("reference", DarkName(element.GetAttribute("reference")));
            }

            XmlElement environment = document.CreateElement("Environment");
            AddColor(document, environment, "Default", "#D4D4D4", "#1E1E1E");
            AddColor(document, environment, "Selection", "#FFFFFF", "#264F78");
            AddColor(document, environment, "VRuler", "#555555", "#1E1E1E");
            AddColor(document, environment, "InvalidLines", "#F44747", null);
            AddColor(document, environment, "CaretMarker", "#292D32", "#1E1E1E");
            AddColor(document, environment, "LineNumbers", "#858585", "#252526");
            AddColor(document, environment, "FoldLine", "#858585", "#1E1E1E");
            AddColor(document, environment, "FoldMarker", "#858585", "#252526");
            AddColor(document, environment, "SelectedFoldLine", "#D4D4D4", null);
            AddColor(document, environment, "EOLMarkers", "#858585", null);
            AddColor(document, environment, "SpaceMarkers", "#858585", null);
            AddColor(document, environment, "TabMarkers", "#858585", null);
            root.PrependChild(environment);

            MemoryStream stream = new MemoryStream();
            document.Save(stream);
            stream.Position = 0;
            return new XmlTextReader(stream);
        }

        private static void AddColor(XmlDocument document, XmlElement environment, string name, string foreground, string background)
        {
            XmlElement element = document.CreateElement(name);
            element.SetAttribute("color", foreground);
            if (background != null)
                element.SetAttribute("bgcolor", background);
            environment.AppendChild(element);
        }

        private static string DarkForeground(string original)
        {
            switch (original.ToLowerInvariant())
            {
                case "black": case "white": return "#D4D4D4";
                case "blue": case "navy": case "darkblue": case "midnightblue": return "#569CD6";
                case "green": case "springgreen": return "#6A9955";
                case "darkgreen": return "#B5CEA8";
                case "gray": case "darkgray": case "silver": case "slategray":
                case "darkslategray": return "#858585";
                case "red": return "#F44747";
                case "darkred": return "#D16969";
                case "brown": case "saddlebrown": case "sienna": case "pink": return "#CE9178";
                case "maroon": case "magenta": case "darkmagenta": case "purple":
                case "darkviolet": case "deeppink": case "#6f002f": case "#900090": return "#C586C0";
                case "cyan": case "darkcyan": case "teal": return "#4EC9B0";
                case "orange": case "olive": case "yellow": case "#ff7700":
                case "#eee0e000": return "#D7BA7D";
                case "systemcolors.windowtext": case "systemcolors.controltext": return "#D4D4D4";
                case "systemcolors.graytext": case "systemcolors.controldark":
                case "systemcolors.controllight": return "#858585";
                case "systemcolors.highlighttext": return "#FFFFFF";
            }
            if (original.StartsWith("#", StringComparison.Ordinal))
            {
                string rgb = original.Length == 9 ? original.Substring(3) : original.Substring(1);
                if (rgb.Length == 6)
                {
                    int red, green, blue;
                    if (Int32.TryParse(rgb.Substring(0, 2), System.Globalization.NumberStyles.HexNumber, null, out red) &&
                        Int32.TryParse(rgb.Substring(2, 2), System.Globalization.NumberStyles.HexNumber, null, out green) &&
                        Int32.TryParse(rgb.Substring(4, 2), System.Globalization.NumberStyles.HexNumber, null, out blue))
                    {
                        if (red * 0.2126 + green * 0.7152 + blue * 0.0722 < 110)
                            return String.Format("#{0:X2}{1:X2}{2:X2}", (red + 255) / 2, (green + 255) / 2, (blue + 255) / 2);
                    }
                }
            }
            return original;
        }
    }
}
