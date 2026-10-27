uses School;

begin
  var число := DecBig('FFFFFFFFFFFFFFFFFFFFFFFF',16);
  Writeln('Число: ',число);
  Writeln('Основание 16: ',число.ToBase(16));
  Writeln('Цифры: ',число.Digits(16));
  Writeln('Обратное преобразование: ',число.Digits(16).DigitsToBigInteger(16));
  Writeln('Байты: ',BinFormat(256bi,3,':'));
  Writeln('НОК массива: ',School.LCM(Arr(18446744073709551616bi,3bi)));
  Writeln('Минимум и максимум: ',MinMax(Arr(-число,0bi,число)));
end.
