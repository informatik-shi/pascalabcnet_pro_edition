begin
  Writeln('Сочетания: ', Arr(1,2,3).Combinations(2).ToArray);
  Writeln('Перестановки: ', Arr(1,2,3).Permutations.ToArray);
  Writeln('Частичные: ', Arr(1,2,3).Permutations(2).ToArray);
  Writeln('Слова: ', 'абв'.Permutations(2).ToArray);
  Writeln('Буквы: ', 'абв'.Combinations(2).ToArray);
  var a := Arr(1,2,3);
  Writeln('Следующая: ', NextPermutation(a), ' ', a);
end.
