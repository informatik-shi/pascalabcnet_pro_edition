begin
  Writeln('Целые: ', LstInt(1, 2, 3));
  Writeln('Строки: ', LstStr('a', 'bc', nil));
  Writeln('Пустые: ', LstInt(), ' ', LstStr());
  var a := Arr(1, 2);
  var l := Lst(a);
  l[0] := 9;
  Writeln('Копия массива: ', a, ' ', l);
  var nested := Lst(l);
  Writeln('Вложенный список: ', nested);
  Writeln('Копия последовательности: ', Lst(l.AsEnumerable));
end.
