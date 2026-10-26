begin
  Writeln('Слова: ','Анна,Борис;Вера'.ToWords(',;'));
  Writeln('Целые: ','10 -3 7'.ToIntegers);
  Writeln('Дроби: ','1,5 2,25'.ToReals(','));
  var lines:=('Первая'#13#10'Вторая'#10).ToLines;
  Writeln('Строк: ',lines.Length);
end.
