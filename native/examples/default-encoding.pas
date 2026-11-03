uses System.Text;

begin
  Writeln('По умолчанию: ', DefaultEncoding.CodePage);
  DefaultEncoding:=Encoding.Unicode;
  var f:=OpenWrite('native-default-encoding.tmp');
  DefaultEncoding:=Encoding.ASCII;
  f.Write('Привет 🌍');
  f.Close;
  Writeln('Открытый файл сохранил Unicode: ',ReadAllText('native-default-encoding.tmp'));
  f.Rewrite(Encoding.GetEncoding(1251));
  f.Write('Привет');
  f.Close;
  f.Reset(Encoding.GetEncoding(1251));
  Writeln('Явная CP1251: ',f.ReadToEnd);
  f.Close;
  DeleteFile('native-default-encoding.tmp');
end.
