begin
  var text := 'Привет';
  var symbols: sequence of char := text;
  Writeln('Символов: ', symbols.Count);
  Writeln('Первые три: ', symbols.Take(3).ToArray);
  Writeln('Последние два: ', text.TakeLast(2).ToList);
  Writeln('Объединение: ', text.Concat('!'+'').Last);
end.
