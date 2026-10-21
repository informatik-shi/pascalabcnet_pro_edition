// Математика и строки из базового модуля PABCSystem.
begin
  var radius := 5.0;
  Writeln('Площадь круга: ', Round(Pi * Sqr(radius), 2));
  Writeln('Sqr(integer) сохраняет точность: ', Sqr(50000));
  Writeln('Целый корень: ', ISqrt(1000000000000));

  var text := 'Привет, PascalABC.NET!';
  Writeln('Длина строки: ', Length(text));
  Writeln('Подстрока: ', Copy(text, 9, 13));
  Writeln('Позиция Pascal: ', Pos('Pascal', text));

  var n := 10;
  Inc(n, 5);
  Dec(n);
  Writeln('После Inc и Dec: ', n);
end.
