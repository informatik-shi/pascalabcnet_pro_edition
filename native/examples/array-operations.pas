begin
  Writeln('Заполнение массива: ', ArrFill(4,7));
  var m := MatrFill(2,3,5);
  Writeln('Заполнение матрицы: ', m);
  Writeln('Размер: ', m.Size);
  m[0,1] := 8;
  Writeln('Транспонирование: ', Transpose(m));
  Writeln('Элементы: ', m.Elements.ToArray);
  Writeln('Равенство массивов: ', ArrEqual(Arr(1,2),Arr(1,2)));
  Writeln('Равенство матриц: ', MatrEqual(m,Transpose(Transpose(m))));
end.
