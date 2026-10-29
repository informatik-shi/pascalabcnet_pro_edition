begin
  var source:=Arr(1,2,3);
  Writeln('В обратном порядке: ', source.AsEnumerable.Reverse.ToArray);
  Writeln('Исходный массив: ', source);
  Writeln('Значение для пустого источника: ', (new integer[0]).DefaultIfEmpty(42).Single);
  Writeln('Совпадают: ', source.SequenceEqual(Seq(1,2,3)));
end.
