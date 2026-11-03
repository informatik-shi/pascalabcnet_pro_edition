begin
  var l := Arr(1,2,1,3).ToList;
  l.Insert(1,7);
  Writeln('Вставка: ', l);
  Writeln('Поиск: ', l.IndexOf(1), ' ', l.LastIndexOf(1));
  Writeln('Удаление: ', l.Remove(1), ' ', l);
  var part := l.GetRange(1,2);
  Writeln('Диапазон: ', part);
  l.RemoveRange(1,2);
  Writeln('Остаток: ', l);
end.
