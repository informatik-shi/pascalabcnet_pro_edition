uses School;
begin
  Writeln(Factorize(360));
  Writeln(School.PrimeFactors(360));
  Writeln((360).PrimeDivisors);
  Writeln((360).PrimeFactorsCount,' ',(360).PrimeDivisorsCount);
  Writeln(Divizors(12),' ',int64(12).Divisors);
  Writeln((360).DivisorsCount);
  Writeln(int64(600851475143).Factorize);
  Writeln(PrimeFactors(MaxInt64));
  Writeln(PrimeDivisorsCount(MaxInt64),' ',DivisorsCount(MaxInt64));
  Writeln(Factorize(0),' ',Divisors(0));
end.
