uses School;
begin
  Writeln('7: ',(7).IsPrime);
  Writeln('9: ',(9).IsPrime);
  Writeln('Большое простое: ',IsPrime(9223372036854775783));
  Writeln('Большое составное: ',IsPrime(3825123056546413051));
end.
