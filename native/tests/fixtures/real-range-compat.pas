uses School;
begin
  Writeln(Range(0,1,0.25).ToArray);
  Writeln(PABCSystem.Range(1,0,-0.25).ToList);
  var s:=Range(0,1,0.1);
  Writeln(s.Count,' ',s.Last=1,' ',s.ElementAt(3)=3*0.1,' ',s.Contains(1.0));
  Writeln(Range(0,1,0.4).Count,' ',Range(0,1,0.6).Count);
  Writeln(Range(0,1,0.500000000000005).Last=1,' ',Range(0,1,0.500000000000006).Count);
  Writeln(Range(0,1,0.499999999999995).Last=1,' ',Range(0,1,0.499999999999994).Last<>1);
  Writeln(Range(1e40,1e40,1e40).Single=1e40,' ',Range(1e40,0,1e40).Any);
  var q:=Range(0,1,0.5);
  foreach var x in q do foreach var y in q do Write(x,':',y,';');
  Writeln;
  Writeln(q.ToArray,' ',q.MinMax);
  var unused:=Range(0,1,0.0);
  Writeln(unused.ElementAtOrDefault(-1));
  Writeln(Range(0.0,MaxInt,1).First);
end.
