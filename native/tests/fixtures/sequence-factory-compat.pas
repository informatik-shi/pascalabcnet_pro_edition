type R = record x: integer; end;
begin
  var a := Arr(1, 2);
  var snapshot := Seq(a);
  a[0] := 9;
  Writeln(snapshot.ToArray, ' ', a);
  Writeln(PABCSystem.Seq(3, 4).ToList);
  Writeln(Seq(new integer[0]).Any);
  Writeln(SeqFill(3, 7).ToArray);
  Writeln(PABCSystem.SeqFill(0, 'ab').FirstOrDefault = nil);
  var r: R;
  r.x := 1;
  var repeated := SeqFill(2, r);
  r.x := 9;
  var copy := repeated.First;
  copy.x := 7;
  Writeln(repeated.First.x, ' ', repeated.Last.x);
  var nested := SeqFill(2, a);
  Writeln(nested.First = nested.Last);
  Writeln(SeqFill(MaxInt, 42).First);
  Writeln(Range(2, 5).ToList);
  foreach var c in Range('a', 'c') do Write(c);
  Writeln;
  Writeln(Range(3, 1).Any, ' ', Range(3, 1) = Range(9, 2));
  Writeln(Range(not(MaxInt), not(MaxInt) + 1).ToArray);
end.
