begin
  Writeln('Квадраты индексов: ', ArrGen(5, i -> i * i));
  Writeln('Индексы со смещением: ', SeqGen(4, i -> i, 10).ToArray);
  Writeln('Фибоначчи: ', SeqGen(8, 0, 1, (a, b) -> a + b).ToArray);
  Writeln('Степени двойки: ', SeqWhile(1, x -> x * 2, x -> x < 20).ToArray);
  Writeln('Повторный перебор: ', 3.Iterate(x -> x + 3).Take(4).ToArray);
  var matrix := MatrGen(2, 3, (row, column) -> row * 10 + column);
  Writeln('Матрица: ', matrix);
end.
