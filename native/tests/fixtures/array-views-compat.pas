begin
  var a := Arr(7,2,9,4);
  Writeln('Индексы: ', a.Indices.ToArray);
  Writeln('Отбор: ', a.Indices(x -> x > 4).ToArray);
  Writeln('Отбор с индексом: ', a.Indices((x,i) -> x > i*3).ToArray);
  var m := MatrGen(2,3,(r,c) -> r*10+c);
  Writeln('Индексы матрицы: ', m.Indices(x -> x >= 10).ToArray);
  Writeln('Диагональ: ', m.Indices((x,r,c) -> r=c).ToArray);
  Writeln('Элемент и координаты: ', m.ElementsWithIndices.ToArray);
  Writeln('По строкам: ', m.ElementsByRow.ToArray);
  Writeln('По столбцам: ', m.ElementsByCol.ToArray);
end.
