begin
  var numbers := Arr(4, 7, 9);
  var items: sequence of integer := numbers;
  Writeln('Непустая: ', items.Any);
  Writeln('Первый: ', items.First);
  Writeln('Последний: ', items.Last);
  Writeln('По индексу 1: ', items.ElementAt(1));
  Writeln('Содержит 7: ', items.Contains(7));
  var empty := new integer[0];
  Writeln('Пустая: ', empty.Any);
  Writeln('По умолчанию: ', empty.FirstOrDefault);
  Writeln('Единственный: ', Arr(42).Single);
  Writeln('Пустой SingleOrDefault: ', empty.SingleOrDefault);
end.
