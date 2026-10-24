begin
  var b:byte:=255;
  var s:shortint:=-128;
  var n:smallint:=-32768;
  Writeln('Значения: ',b,' ',s,' ',n);
  Writeln('Сумма с расширением: ',b+1);
  Writeln('Квадрат: ',Sqr(b));
  Write('Граница byte: ');
  for var value:byte:=254 to 255 do Write(value,'|');
  Writeln;
  var values:=Arr(byte(1),shortint(-2));
  Writeln('Массив smallint: ',values[0],' ',values[1]);
end.
