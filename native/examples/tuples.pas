uses School;
function Данные: (integer,string);
begin
  Result := (42,'ответ');
end;
begin
  var пара := Данные;
  Writeln(пара.Item2,': ',пара[0]);
  var результат := НОДНОК(18,30);
  Writeln('НОД и НОК: ',результат);
  Writeln('НОД: ',результат.Item1,'; НОК: ',результат.Item2);
end.
