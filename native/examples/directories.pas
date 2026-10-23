begin
  var home:=GetCurrentDir;
  MkDir('Пример каталогов/Документы');
  ChDir('Пример каталогов/Документы');
  WriteAllText('заметка.txt','Файл в выбранной папке');
  Writeln(ReadAllText('заметка.txt'));
  Writeln('Новое имя: ',ChangeFileNameExtension('заметка.txt','bak'));
  Writeln('Файл удалён: ',DeleteFile('заметка.txt'));
  ChDir(home);
  Writeln('Вложенная папка удалена: ',RemoveDir('Пример каталогов/Документы'));
  Writeln('Папка примера удалена: ',RemoveDir('Пример каталогов'));
end.
