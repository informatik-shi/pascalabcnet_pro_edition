uses System.Text;

begin
  var name:='native-file-lines.tmp';
  WriteAllLines(name,Arr('Первая строка','','Последняя строка'),Encoding.GetEncoding(1251));
  Writeln('CP1251: ',PABCSystem.ReadAllLines(name,Encoding.GetEncoding(1251)));
  WriteLines(name,Range(1,3).Select(n->'Строка '+IntToStr(n)),Encoding.Unicode);
  Writeln('Unicode: ',ReadAllLines(name));
  DeleteFile(name);
end.
