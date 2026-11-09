begin
  var e := Encoding.GetEncoding('gb18030');
  Writeln(e.CodePage, ' ', e.GetType.Name);
  var text := '中文 日本語 한국어 🌍';
  Writeln(e.GetString(e.GetBytes(text)));
  Writeln(e.GetBytes('🌍'));
  Writeln(e.GetBytes(#128));
end.
