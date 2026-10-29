function Square(value:integer):integer;
begin Result:=value*value; end;
function Even(value:integer):boolean;
begin Result:=value mod 2=0; end;
begin
  var squares:=Range(1,10).Where(Even).Select(Square);
  Writeln('Квадраты чётных: ', squares.ToArray);
  Writeln('Первые два: ', squares.Take(2).ToList);
  Writeln('Сумма: ', squares.Sum);
end.
