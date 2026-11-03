uses System.Text;

begin
  var utf8: Encoding := Encoding.UTF8;
  var cyrillic := Encoding.GetEncoding('windows-1251');
  Writeln('Кодировки: ', utf8.CodePage, ' ', cyrillic.CodePage);
  var bytes := cyrillic.GetBytes('Привет');
  Writeln(bytes);
  Writeln(cyrillic.GetString(bytes));
  WriteAllText('native-encoding-demo.tmp', 'Привет 🌍', Encoding.Unicode);
  Writeln(ReadAllText('native-encoding-demo.tmp', utf8));
  DeleteFile('native-encoding-demo.tmp');
end.
