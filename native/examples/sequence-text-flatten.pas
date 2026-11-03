begin
  Writeln('Числа: ', Seq(1,2,3).JoinToString(', '));
  Writeln('Символы: ', Arr('a','b','c').JoinToString);
  var rows := Arr(Arr(1,2),Arr(3),new integer[0]);
  Writeln('Массивы: ', rows.Flatten.ToArray);
  Writeln('Строки: ', Seq('Pascal','ABC').Flatten.JoinIntoString);
  var text := Arr(nil,'abc','',nil);
  Writeln('Пустые значения: [', text.JoinToString('|'), ']');
end.
