begin
  Writeln('AND: ',12 and 10);
  Writeln('OR: ',12 or 10);
  Writeln('XOR: ',12 xor 10);
  var mask:longword:=2147483648;
  Writeln('Беззнаковый сдвиг: ',mask shr 31);
  Writeln('Знаковый сдвиг: ',-8 shr 1);
  var b:byte:=0;
  Writeln('Инверсия byte: ',not b);
  Writeln('Логический XOR: ',true xor false);
end.
