program Sum;
var n: integer;
begin
  Write('Введите n: ');
  Readln(n);
  var i := 1;
  var total: int64 := 0;
  while i <= n do
  begin
    total := total + i;
    i := i + 1;
  end;
  Writeln('Сумма: ', total);
end.
