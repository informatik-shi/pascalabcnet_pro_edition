uses System;
var trace: string;
function Track(x:integer):integer;
begin
  trace:=trace+IntToStr(x);
  Result:=x;
end;
begin
  var s:=SSetInt(3,1,2);
  Writeln(s.SetEquals(Arr(3,1,2,2)),' ',s.Overlaps(Arr(2,8)),' ',s.IsProperSubsetOf(Arr(5,2,1,3)),' ',s.IsProperSupersetOf(Arr(1,1,2)));
  Writeln(s.IsSubsetOf(s),' ',s.IsSupersetOf(s),' ',s.IsProperSubsetOf(s),' ',s.IsProperSupersetOf(s));
  var absCmp:=System.Collections.Generic.Comparer&<integer>.Create((a,b) -> Abs(a)-Abs(b));
  var byAbs:=new SortedSet<integer>(Arr(1,2,3),absCmp);
  var signed:=SSetInt(-1,-2,-3);
  Writeln(byAbs.SetEquals(signed),' ',signed.SetEquals(byAbs),' ',byAbs.IsSupersetOf(Arr(-2,-3)));
  trace:='';
  Writeln(s.SetEquals(Seq(1,4,2,3).Select(Track)),' ',trace);
  trace:='';
  Writeln(s.IsSubsetOf(Seq(1,2,3,4).Select(Track)),' ',trace);
  trace:='';
  Writeln(s.Overlaps(Seq(4,3,2,1).Select(Track)),' ',trace);
  trace:='';
  Writeln(s.IsProperSupersetOf(Seq(2,4,3).Select(Track)),' ',trace);
  var e:=SSetInt();
  trace:='';
  Writeln(e.IsProperSubsetOf(Seq(2,1).Select(Track)),' ',trace);
  trace:='';
  Writeln(e.IsSubsetOf(Seq(2,1).Select(Track)),' [',trace,']');
end.
