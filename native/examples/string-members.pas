begin
  var text:='12 3.5 слово'; var position:=0;
  Writeln('Целое: ',text.ReadInteger(position));
  Writeln('Дробь: ',text.ReadReal(position));
  Writeln('Слово: ',text.ReadWord(position));
  Writeln('По умолчанию: ','ошибка'.ToInteger(7));
  Writeln('С запятой: ','1,5'.ToReal(','));
end.
