begin
  var start := Milliseconds;
  Sleep(5);
  Writeln('Пауза выполнена: ', Milliseconds-start>=5);
  var interval := MillisecondsDelta;
  Sleep(0);
  Writeln('Интервалы неотрицательны: ', interval>=0, ' ', MillisecondsDelta>=0);
  Writeln('Счётчик доступен: ', PABCSystem.Milliseconds()>=0);
end.
