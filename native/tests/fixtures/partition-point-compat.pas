uses School;
begin
  Writeln(PartitionPoints(-1,1,4).ToArray);
  Writeln(PABCSystem.Linspace(1,0,5).ToList);
  Writeln(Linspace(2,2,3).ToArray);
  var s:=PartitionPoints(1,3,2);
  foreach var x in s do foreach var y in s do Write(x,':',y,';');
  Writeln;
  Writeln(s.ToArray,' ',s.MinMax);
  Writeln(Linspace(0,1,11).Last<>1);
  var invalid:=PartitionPoints(0,1,0);
  Writeln(invalid.ElementAtOrDefault(-1));
  Writeln(PartitionPoints(0,1,MaxInt).First);
end.
