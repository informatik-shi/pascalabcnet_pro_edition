uses School;
begin
  Writeln('Множители 360: ',Factorize(360));
  Writeln('Простые делители: ',(360).PrimeDivisors);
  Writeln('Делители 12: ',Divisors(12));
  Writeln('Число делителей 360: ',(360).DivisorsCount);
  Writeln('Большое число: ',int64(600851475143).Factorize);
end.
