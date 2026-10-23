begin
  var f:Text;
  Assign(f,'Исходная заметка.txt');
  f.Rewrite;
  f.Println('Записано на C++');
  f.Close;

  f.Rename('Новая заметка.txt');
  Writeln('Файл переименован: ',FileExists('Новая заметка.txt'));
  Writeln('Имя в переменной: ',f.Name);
  Assign(f,'Новая заметка.txt');
  f.Reset;
  Writeln(f.ReadString);
  f.Close;
  f.Erase;
  Writeln('Файл удалён: ',not FileExists('Новая заметка.txt'));
end.
