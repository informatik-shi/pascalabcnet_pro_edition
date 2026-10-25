begin
  var number:=0;
  if TryRead(number,'Введите целое число:') then
    Writeln('Прочитано: ',number)
  else
    Writeln('Некорректный ввод. Значение: ',number);
end.
