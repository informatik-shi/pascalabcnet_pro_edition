uses School;
begin
  Writeln(Range(2bi,5bi).ToArray);
  Writeln(PABCSystem.Range(8bi,2bi,-3bi).ToList);
  Writeln(Range(MaxUInt64,18446744073709551617bi).ToArray);
  var s:=Range(-2bi,2bi,2bi);
  foreach var x in s do foreach var y in s do Write(x,':',y,';');
  Writeln;
  Writeln(s.ToArray,' ',s.MinMax);
  Writeln(s.ElementAt(1),' ',s.Contains(2bi),' ',Range(1bi,0bi).SingleOrDefault);
end.
