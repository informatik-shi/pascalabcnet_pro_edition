function Even(value:integer):boolean;
begin Result:=value mod 2=0; end;
function Square(value:integer):integer;
begin Result:=value*value; end;
begin
  var source:=Range(1,10);
  Writeln('Есть чётные: ', source.Any(Even));
  Writeln('Чётных чисел: ', source.Count(Even));
  Writeln('Первое чётное: ', source.First(Even));
  Writeln('Сумма квадратов: ', source.Sum(Square));
end.
