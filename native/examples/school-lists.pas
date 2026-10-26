uses School;
begin
  var digits := Digits(2026);
  Writeln('Цифры: ',digits);
  Writeln('Обратно: ',digits.DigitsToInt64);
  Writeln('Простые до 30: ',Primes(30));
  Writeln('Первые 5: ',FirstPrimes(5));
end.
