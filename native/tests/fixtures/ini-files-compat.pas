uses IniFile;
begin
  var config:=new TIniFile('example-settings.ini');
  config.WriteString('Приложение','Имя','ABC Native');
  config.WriteInteger('Окно','Ширина',1000);
  config.WriteBoolean('Окно','Ночная тема',True);
  config.Save;
  var loaded:=TIniFile.Create('example-settings.ini');
  Writeln('Файл: ',loaded.FileName);
  Writeln('Имя: ',loaded.ReadString('Приложение','Имя','?'));
  Writeln('Ширина: ',loaded.ReadInteger('Окно','Ширина',800));
  Writeln('Ночная тема: ',loaded.ReadBoolean('Окно','Ночная тема',False));
  Writeln('Нет ключа: ',loaded.ReadString('Окно','Положение','По умолчанию'));
end.
