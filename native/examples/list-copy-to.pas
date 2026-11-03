begin
  var l := Lst(1, 2, 3);
  var a := new integer[5];
  l.CopyTo(a, 1);
  Writeln('Весь список: ', a);
  l.CopyTo(1, a, 0, 2);
  Writeln('Диапазон: ', a);
  Writeln('Исходный список: ', l);
end.
