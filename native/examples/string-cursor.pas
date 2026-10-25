begin
  var position:=1;
  var line:='12 3.5 слово';
  Writeln('Целое: ',ReadIntegerFromString(line,position));
  Writeln('Дробь: ',ReadRealFromString(line,position));
  Writeln('Слово: ',ReadWordFromString(line,position));
  Writeln('Конец: ',StringIsEmpty(line,position));
end.
