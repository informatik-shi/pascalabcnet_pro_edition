begin
  var m := MatrGen(2,3,(r,c) -> r*10+c);
  Writeln('Строка: ', m.Row(1));
  Writeln('Столбец: ', m.Col(2));
  Writeln('Строки: ', m.Rows);
  Writeln('Столбцы: ', m.Cols);
  Writeln('Ленивые строки: ', m.RowsSeq.Select(row -> row.ToArray).ToArray);
  Writeln('Ленивые столбцы: ', m.ColsSeq.Select(col -> col.ToArray).ToArray);
  m.SwapRows(0,1);
  m.SwapCols(0,2);
  Writeln('После обменов: ', m);
  m.SetRow(0,Seq(7,8,9));
  m.SetCol(1,Arr(4,5));
  Writeln('После замены: ', m);
end.
