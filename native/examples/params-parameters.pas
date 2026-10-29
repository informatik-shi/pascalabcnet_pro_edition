function SumValues(params values:array of integer):integer;
begin Result:=values.Sum; end;
begin
  Writeln('Без аргументов: ', SumValues);
  Writeln('Три числа: ', SumValues(1,2,3));
  var numbers:=Arr(10,20,30);
  Writeln('Готовый массив: ', SumValues(numbers));
end.
