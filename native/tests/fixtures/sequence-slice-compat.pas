begin
  var a:=Arr(1,2,3,4,5);
  Writeln(a.Take(2).ToArray,' ',PABCSystem.Skip(a,2).ToList);
  Writeln(TakeLast(a,2).ToArray,' ',a.SkipLast(2).ToArray,' ',a.SkipLast.ToArray);
  Writeln(a.Take(0).Count,' ',a.Skip(-1).ToArray,' ',a.TakeLast(-1).Any);
  var nils:sequence of integer;
  Writeln(nils.TakeLast(0).Count);
  var deferred:=nils.SkipLast(1);
  Writeln(deferred<>nil,' ',deferred.ElementAtOrDefault(-1));
  Writeln(Range(MaxInt,MaxInt,1).Take(1).Single);
  Writeln(Range(0,1,0.0).Take(0).Any);
  Writeln(Range(MaxInt-1,MaxInt,1).SkipLast(1).First);
  Writeln(Seq(1,2,3).PartialSum.Skip(1).Take(1).Single);
  Writeln(Seq('ab','cd','ef').TakeLast(2).ToArray);
end.
