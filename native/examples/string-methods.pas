begin
  var имя := 'Анна';
  var номер := '42';
  Writeln('Имя: ',имя.PadRight(8,'.'));
  Writeln('Номер: ',номер.PadLeft(5,'0'));
  var текст := 'abcde';
  Writeln('Подстрока: ',текст.Substring(2));
  Writeln('Удаление: ',текст.Remove(2));
  Writeln('Вставка: ',текст.Insert(1,'X'));
  Writeln('Исходная строка: ',текст);
end.
