begin
  var numbers := Arr(1, 2);
  var snapshot := Seq(numbers);
  numbers[0] := 9;
  Writeln('Снимок: ', snapshot.ToArray);
  Writeln('Исходный массив: ', numbers);
  Writeln('Повторения: ', SeqFill(3, 'да').ToList);
  var huge := SeqFill(MaxInt, 7);
  Writeln('Первый большого повтора: ', huge.First);
  Writeln('Диапазон: ', Range(2, 5).ToArray);
  Write('Символы: ');
  foreach var character in Range('a', 'c') do Write(character);
  Writeln;
end.
