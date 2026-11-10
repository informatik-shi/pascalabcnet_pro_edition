begin
  var e := Encoding.GetEncoding(65000);
  Writeln(e.CodePage, ' ', e.GetType.Name);
  Writeln(e.GetPreamble.Length);
  Writeln(e.GetBytes('A+日').JoinIntoString(','));
  Writeln(e.GetString(e.GetBytes('Привет + 日本 🌍')));
  var lone := e.GetString(e.GetBytes(#55357));
  Writeln(lone.Length, ' ', Ord(lone[1]));
end.