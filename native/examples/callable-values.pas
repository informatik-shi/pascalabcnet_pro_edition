type Transform = integer → integer;

function Twice(x: integer): integer;
begin
  Result := x * 2;
end;

function Apply(f: Transform; x: integer): integer;
begin
  Result := f(x);
end;

begin
  var f: Transform := Twice;
  Writeln('Значение: ', f(7));
  var functions := Arr(Twice, Twice);
  Writeln('Через массив: ', functions[0](9));
  Writeln('Через параметр: ', Apply(f, 11));
  f := nil;
  Writeln('Пустое значение: ', f = nil);
end.
