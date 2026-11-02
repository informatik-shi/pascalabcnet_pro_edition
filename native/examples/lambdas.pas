function Counter(start: integer): () -> integer;
begin
  var current := start;
  Result := () -> begin
    Inc(current);
    Result := current;
  end;
end;

begin
  Writeln('Квадраты: ', Seq(1, 2, 3).Select(x -> x * x).ToArray);
  var threshold := 2;
  var greater := Seq(1, 2, 3).Where(x -> x > threshold);
  threshold := 1;
  Writeln('Изменяемый захват: ', greater.ToArray);
  var counter := Counter(10);
  Writeln('Счётчик: ', counter(), ' ', counter());
  var add := (a: integer) -> (b: integer) -> a + b;
  Writeln('Вложенные функции: ', add(4)(5));
end.
