uses School;
begin
  Writeln('Последовательная замена: ',ReplaceMany('ab','ab','bc'));
  Writeln('Посимвольный перевод: ',Translate('ab','ab','bc'));
  var text := 'раз-два-раз';
  ReplaceLast(text,'раз','три');
  Writeln('Последнее вхождение: ',text);
  Writeln('Подстроки: ',ReplaceMany('cat dog',Arr('cat','dog'),Arr('кот','пёс')));
  Writeln('Числа int64: ','2147483648 -7'.ToInts64);
  Writeln('Импликация: ',True.Imp(False));
end.
