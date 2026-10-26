uses School;
begin
  Writeln(Digits(0));
  Writeln(Digits(-7));
  Writeln(Digits(255,16));
  var d: List<integer> := School.Digits(12345);
  d[0] := 9;
  Writeln(d,' ',d.DigitsToInt64);
  Writeln(Primes(20));
  Writeln(Primes(20,2));
  Writeln(Primes(2147483600,MaxInt));
  Writeln(FirstPrimes(5));
  var cache := FirstPrimes(4794);
  Writeln(cache.Count,' ',cache[4793]);
  cache[0] := 17;
  Writeln(FirstPrimes(3));
  var second := FirstPrimes(4794);
  Writeln(cache=second,' ',second.Count);
end.
