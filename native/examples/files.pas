// Программа создаёт файлы native-example.txt и native-lines.txt в папке запуска.
begin
  WriteAllText('native-example.txt','Привет из C++ интерпретатора!');
  Writeln(ReadAllText('native-example.txt'));

  var lines := Arr('Первая строка','Вторая строка');
  WriteAllLines('native-lines.txt',lines);
  var restored := ReadAllLines('native-lines.txt');
  Writeln('Прочитано строк: ',Length(restored));
  Writeln(restored[1]);
end.
