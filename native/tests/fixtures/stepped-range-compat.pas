uses School;
begin
  Writeln(Range(1,10,3).ToArray);
  Writeln(PABCSystem.Range(10,1,-3).ToList);
  Writeln(Range('а','ж',2).ToArray);
  Writeln(Range('z','t',-2).ToArray);
  Writeln(Range(1,5,-1).Count,' ',Range(5,1,1).Any);
  var s:=Range(-2,2,2);
  foreach var x in s do Write(x,';');
  Writeln;
  Writeln(s.ToArray,' ',s.MinMax);
  Writeln(Range(not(MaxInt),0,MaxInt).ToArray);
  Writeln(Range(MaxInt,0,not(MaxInt)).ToArray);
  Writeln(Range(MaxInt,MaxInt,1).First);
  foreach var x in Range(not(MaxInt),not(MaxInt),-1) do
  begin
    Writeln(x);
    break;
  end;
  var unused:=Range(1,2,0);
  Writeln('deferred ',unused.ElementAtOrDefault(-1));
end.
