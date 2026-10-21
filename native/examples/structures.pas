type
  TPoint = record x,y: integer; end;

function Factorial(n: integer): int64;
begin
  if n <= 1 then Exit(1);
  Result := n * Factorial(n-1);
end;

procedure Move(var point: TPoint; dx,dy: integer);
begin
  point.x := point.x + dx;
  point.y := point.y + dy;
end;

begin
  var point: TPoint;
  Move(point,3,4);
  Writeln('Точка: ',point.x,' ',point.y);
  Writeln('Факториал 10: ',Factorial(10));

  var numbers := Arr(5,2,8,1);
  Sort(numbers);
  var i := 0;
  while i < Length(numbers) do begin
    Writeln(numbers[i]);
    Inc(i);
  end;

  var names := new List<string>;
  names.Add('Анна'); names.Add('Борис');
  Writeln('Имен: ',names.Count);
  Writeln('Первое имя: ',names[0]);
end.
