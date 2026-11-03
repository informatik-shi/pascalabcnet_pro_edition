begin
  var m := MatrGen(3,4,(r,c) -> r*10+c);
  Writeln('Матрица: ', m);
  Writeln('Индексы: ', m.MatrSlice(Arr(2,0,2),Arr(3,1)));
  Writeln('Границы: ', MatrSlice(m,1,2,1,3));
  Writeln('Пустые строки: ', m.MatrSlice(1,0,0,3).Size);
  Writeln('Пустые столбцы: ', m.MatrSlice(new integer[0],Arr(1,2)).Size);
end.
