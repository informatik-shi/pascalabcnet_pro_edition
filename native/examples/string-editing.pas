// Изменяемые строки: позиции и длина считаются в UTF-16, начиная с 1.
begin
  var text := 'Привет, мир';
  Insert('новый ', text, 9);
  Writeln('После Insert: ', text);
  Delete(text, 9, 6);
  Writeln('После Delete: ', text);

  SetLength(text, 6);
  Writeln('После сокращения: [', text, ']');
  SetLength(text, 9);
  Writeln('После дополнения пробелами: [', text, ']');
end.
