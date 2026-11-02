begin
  Writeln('Пары: ', Cartesian(Seq(1, 2), 'ab').ToArray);
  Writeln('Суммы: ', Cartesian(Seq(1, 2), Seq(10, 20), (a, b) -> a + b).ToArray);
  Writeln('Тройки: ', Arr(0, 1).CartesianPower(3).ToArray);
  Writeln('Слова: ', 'аб'.Cartesian(2).ToArray);
  var numbers := SeqGen(2, i -> i + 1);
  Writeln('Степень последовательности: ', numbers.CartesianPower(2).ToArray);
end.
