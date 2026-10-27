uses School;
begin
  var числа := Arr(12,-7,42,5);
  var границы := числа.MinMax;
  Writeln('Минимум: ',границы.Item1);
  Writeln('Максимум: ',границы.Item2);
  Writeln('Пара: ',границы);
end.
