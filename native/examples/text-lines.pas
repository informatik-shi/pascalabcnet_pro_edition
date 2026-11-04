begin
  var name:='native-text-lines.tmp';
  WriteAllText(name,'Первая строка'#10'Вторая строка'#10);
  var f:=OpenRead(name);
  Writeln('Массив: ',f.Lines.ToArray);
  f.Reset;
  foreach var line in f.Lines do Writeln('Строка: ',line);
  Writeln('Конец файла: ',f.Eof);
  f.Close;Writeln('После Close: ',DeleteFile(name));
end.
