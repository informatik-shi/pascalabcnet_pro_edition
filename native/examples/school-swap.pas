uses School;
begin
  var text: string := 'кот пёс кот';
  SwapSubstr(text, 'кот', 'пёс');
  Writeln('Обмен: ', text);
  text.SwapSubstr('кот', 'пёс');
  Writeln('Обратно: ', text);
  text := 'a';
  SwapSubstr(text, '', 'a');
  Writeln('Пустые совпадения: ', text);
end.
