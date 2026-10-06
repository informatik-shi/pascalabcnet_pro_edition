using System;
using System.Text.RegularExpressions;
using ICSharpCode.TextEditor;
using ICSharpCode.TextEditor.Document;

namespace VisualPascalABC
{
    /// <summary>Smart indentation for the SPython editor (.pys files).</summary>
    public sealed class SPythonFormattingStrategy : DefaultFormattingStrategy
    {
        private static readonly Regex BlockHeader = new Regex(
            @"^(?:(?:async\s+(?:def|for|with))|(?:if|elif|else|for|while|def|class|try|except|finally|with|match|case))\b",
            RegexOptions.CultureInvariant);

        protected override int SmartIndentLine(TextArea textArea, int line)
        {
            int inheritedIndent = base.SmartIndentLine(textArea, line);
            if (line == 0 || !ShouldIncreaseIndent(TextUtilities.GetLineAsString(textArea.Document, line - 1)))
                return inheritedIndent;

            int indentSize = textArea.Document.TextEditorProperties.IndentationSize;
            if (indentSize < 1)
                indentSize = 4;

            string current = TextUtilities.GetLineAsString(textArea.Document, line);
            int leadingLength = 0;
            while (leadingLength < current.Length &&
                   (current[leadingLength] == ' ' || current[leadingLength] == '\t'))
                leadingLength++;

            string indentation = GetIndentation(textArea, line - 1) + new string(' ', indentSize);
            LineSegment segment = textArea.Document.GetLineSegment(line);
            textArea.Document.Replace(segment.Offset, leadingLength, indentation);
            return indentation.Length;
        }

        // A colon in a string, slice, dict, or inline suite does not open a new block.
        public static bool ShouldIncreaseIndent(string line)
        {
            if (String.IsNullOrWhiteSpace(line))
                return false;

            string trimmed = line.TrimStart();
            if (!BlockHeader.IsMatch(trimmed))
                return false;

            char quote = '\0';
            bool triple = false;
            int bracketDepth = 0;
            char lastCodeCharacter = '\0';
            for (int i = 0; i < trimmed.Length; i++)
            {
                char ch = trimmed[i];
                if (quote != '\0')
                {
                    if (ch == '\\' && i + 1 < trimmed.Length) { i++; continue; }
                    if (ch == quote)
                    {
                        if (triple)
                        {
                            if (i + 2 < trimmed.Length && trimmed[i + 1] == quote && trimmed[i + 2] == quote)
                            { quote = '\0'; triple = false; i += 2; }
                        }
                        else quote = '\0';
                    }
                    continue;
                }

                if (ch == '#')
                    break;
                if (ch == '\'' || ch == '"')
                {
                    quote = ch;
                    triple = i + 2 < trimmed.Length && trimmed[i + 1] == ch && trimmed[i + 2] == ch;
                    if (triple) i += 2;
                    lastCodeCharacter = ch;
                    continue;
                }
                if (ch == '(' || ch == '[' || ch == '{') bracketDepth++;
                else if (ch == ')' || ch == ']' || ch == '}') bracketDepth = Math.Max(0, bracketDepth - 1);
                if (!Char.IsWhiteSpace(ch))
                    lastCodeCharacter = bracketDepth == 0 ? ch : '\0';
            }
            return lastCodeCharacter == ':' && bracketDepth == 0 && quote == '\0';
        }
    }
}
