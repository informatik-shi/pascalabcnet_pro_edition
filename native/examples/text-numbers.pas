// Строки и преобразования чисел: собственная реализация на C++.
begin
  var text := '  Привет, Pascal!  ';
  Writeln('[', Trim(text), ']');
  Writeln('Начало: ', LeftStr(Trim(text), 6));
  Writeln('Конец: ', RightStr(Trim(text), 7));
  Writeln('Наоборот: ', ReverseString('Pascal'));

  var count: integer := StrToInt(' +42 ');
  var large: int64 := StrToInt64('9223372036854775807');
  var price: real := StrToReal('123,45', ',');
  Writeln('Целое: ', IntToStr(count));
  Writeln('Большое целое: ', IntToStr(large));
  Writeln('Цена: ', FloatToStr(price));
  Writeln('Сравнение: ', CompareStr('abc', 'abd') < 0);
end.
