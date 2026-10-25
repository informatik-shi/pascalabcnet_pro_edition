begin
  var s:string;
  Str(Pi:8:3,s);
  Writeln('Форматирование: [',s,']');
  var number:=10; var code:integer;
  Val('ошибка',number,code);
  Writeln('Ошибка: ',code,'; прежнее значение: ',number);
  if '42'.TryToInteger(number) then Writeln('Прочитано: ',number);
end.
