begin
  Writeln('Вверх: ',Range(1,10,3).ToArray);
  Writeln('Вниз: ',Range(10,1,-3).ToList);
  Write('Символы: ');
  foreach var c in Range('а','ж',2) do Write(c);
  Writeln;
  var whole:=Range(not(MaxInt),MaxInt,1);
  Writeln('Начало большого диапазона: ',whole.First);
  foreach var x in Range(MaxInt,MaxInt,1) do
  begin
    Writeln('Последнее значение перед break: ',x);
    break;
  end;
end.
