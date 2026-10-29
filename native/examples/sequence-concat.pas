begin
  var first := Range(1, 3);
  var second := Seq(4, 5);
  var combined := 0 + first.Concat(second) + 6;
  Writeln('Последовательность: ', combined.ToArray);
  Writeln('Повторный обход: ', combined.ToList);
  Writeln('Первые два: ', combined.Take(2).ToArray);
  Writeln('Сумма: ', combined.Sum);
end.
