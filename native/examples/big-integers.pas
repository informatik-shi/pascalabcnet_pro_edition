begin
  var факториал: BigInteger := 1;
  for var i := 2 to 100 do
    факториал := факториал * i;
  Writeln('100! = ',факториал);
  var число := BigInteger.Parse('123456789012345678901234567890');
  Writeln('Целый корень квадрата: ',ISqrt(число*число));
  Writeln('НОД: ',GCD(48bi,18bi));
  Writeln('Сдвиги: ',(1bi shl 80) shr 76);
end.
