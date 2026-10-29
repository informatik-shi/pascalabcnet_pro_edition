function Parts(value:integer):sequence of integer;
begin Result:=Range(1,value); end;
begin
  var s:=Seq(1,2,3).SelectMany(Parts);
  Writeln('Все элементы: ', s.ToArray);
  Writeln('Первые четыре: ', s.Take(4).ToList);
  Writeln('Сумма: ', s.Sum);
end.
