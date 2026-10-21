uses Collections;
begin
  var numbers := new DynArray<integer>;
  numbers.Add(10); numbers.Add(30);
  numbers.Insert(1,20);
  Writeln('Массив: ',numbers.ToString);
  Writeln('Размер: ',numbers.Count,'; ёмкость: ',numbers.Capacity);
  numbers.Remove(0);
  Writeln('После удаления: ',numbers.ToString);

  var names := new SimpleSet<string>;
  names.Add('Анна'); names.Add('Борис'); names.Add('Анна');
  Writeln('Без повторов: ',names.ToString);
  names.Remove('Анна');
  Writeln('Есть Анна: ',names.Contains('Анна'));
end.
