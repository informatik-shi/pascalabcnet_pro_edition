begin
  var home:=GetDir;
  MkDir('Пример переименования/Черновики');
  ChDir('Пример переименования');
  WriteAllText('Черновики/заметка.txt','Текст заметки');
  Writeln('Файл переименован: ',RenameFile('Черновики/заметка.txt','Черновики/готово.txt'));
  Writeln('Каталог переименован: ',RenameDirectory('Черновики','Документы'));
  Writeln(ReadAllText('Документы/готово.txt'));
  WriteAllText('Документы/занято.txt','Существующий текст');
  Writeln('Занятое имя: ',RenameFile('Документы/готово.txt','Документы/занято.txt'));
  Writeln(ReadAllText('Документы/занято.txt'));
  DeleteFile('Документы/готово.txt'); DeleteFile('Документы/занято.txt');
  RmDir('Документы'); ChDir(home); RmDir('Пример переименования');
end.
