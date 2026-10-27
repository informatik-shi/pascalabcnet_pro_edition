begin
  var text: string := 'Заказ 123, строка 456';
  Writeln('Есть число: ', text.IsMatch('\d+'));
  Writeln('Первое число: ', text.MatchValue('\d+'));
  Writeln('Без числа: [', MatchValue('Привет', '\d+'), ']');
  Writeln('Проверка: ', Regex.IsMatch('abc', '^a'));
end.
