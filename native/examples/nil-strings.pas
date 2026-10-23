uses Collections;
begin
  var names:=new AssocArray<string,string>;
  var missing:=names['нет ключа'];
  Writeln('Нет значения: ',missing);
  Writeln('Это nil: ',missing=nil);
  names['пустое имя']:='';
  Writeln('Пустая строка — nil: ',names['пустое имя']=nil);
  Writeln('Длина nil: ',Length(missing));
  Writeln('Соединение: [',missing+'текст',']');
  Insert('Новое имя',missing,1);
  Writeln('После Insert: ',missing);
end.
