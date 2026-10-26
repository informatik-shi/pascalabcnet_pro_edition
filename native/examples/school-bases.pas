uses School;
begin
  Writeln('Двоичная: ',Bin(42));
  Writeln('Шестнадцатеричная: ',Hex(255));
  Writeln('Из HEX: ',School.Dec('FF',16));
  Writeln('Байты: ',BinFormat(258));
  Writeln('Корень: ',PerfectSquare(81));
  Writeln('Синус 30: ',Round(SinDegrees(30),3));
end.
