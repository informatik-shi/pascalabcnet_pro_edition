begin
  var numbers := Arr(1, 2);
  var list := numbers.ToList();
  list[0] := 9;
  list.Add(3);
  Writeln('Массив: ', numbers);
  Writeln('Список: ', list);
  var sequence: sequence of integer := list;
  var copy := sequence.ToList;
  list[1] := 8;
  Writeln('Копия: ', copy);
  Writeln('После изменения: ', list);
end.
