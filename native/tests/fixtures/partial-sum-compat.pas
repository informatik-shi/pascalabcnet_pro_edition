uses School;
begin
  var s:=Arr(1,2,3,-4).PartialSum;
  Writeln(s.ToArray,' ',s.Sum,' ',s.Average);
  Writeln(s.PartialSum.ToArray);
  var a:=Arr(1,2,3);
  var p:=a.PartialSum;
  a[0]:=9;
  Writeln(p.ToArray,' ',p.ToArray);
  Writeln(Arr(0.25,0.5,0.75).PartialSum.ToList);
  Writeln(Arr(1bi,2bi,3bi).PartialSum.ToArray);
  Writeln(Range(1,3).PartialSum.MinMax);
  var empty:array of integer;
  var deferred:=empty.PartialSum;
  Writeln(deferred<>nil,' ',deferred.ElementAtOrDefault(-1));
  var overflow:=Arr(MaxInt,1).PartialSum;
  Writeln(overflow.First,' ',overflow.Contains(MaxInt));
  foreach var x in overflow do
  begin
    Writeln(x);
    break;
  end;
end.
