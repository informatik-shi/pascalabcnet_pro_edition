uses FilesOperations;
begin
  var name:='Заметка FilesOperations.txt';
  WriteStringToFile(name,'Первая строка'#10'Вторая строка');
  Writeln('Прочитано:');
  Write(FilesOperations.ReadFileToEnd(name));
  Writeln('Файл удалён: ',DeleteFile(name));
end.
