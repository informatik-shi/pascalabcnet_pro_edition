type F = integer -> integer;
begin
  var empty := Lst&<integer>;
  Writeln('Пустой типизированный: ', empty);
  empty.Add(7);
  Writeln('Добавление: ', empty);
  var functions := Lst&<F>(x -> x + 1);
  Writeln('Функция: ', functions[0](2));
  Writeln('Символы: ', Lst&<char>('abc'));
  Writeln('Строка: ', Lst&<string>('abc'));
end.
