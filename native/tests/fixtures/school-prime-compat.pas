uses School;
begin
  for var n:=0 to 20 do if n.IsPrime then Write(n,' ');
  Writeln;
  Writeln(IsPrime(2147483647),' ',IsPrime(4294967297));
  Writeln(IsPrime(2305843009213693951),' ',IsPrime(9223372036854775783));
  Writeln(School.IsPrime(3825123056546413051),' ',School.IsPrime(MaxInt64));
  SetRandomSeed(1); IsPrime(2305843009213693951); Writeln(Random(100));
end.
