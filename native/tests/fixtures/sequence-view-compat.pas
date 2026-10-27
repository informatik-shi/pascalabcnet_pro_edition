uses School;
type R=record x: integer; end;
begin
  var a:=Arr(7,-3,9);
  var s:sequence of integer:=a;
  a[0]:=2;
  Writeln(s.ToArray,' ',s.MinMax);
  var t:sequence of integer:=a;
  Writeln(s=t);
  var l:=new List<integer>;
  l.Add(4);
  var q:sequence of integer:=l;
  l.Add(5);
  Writeln(Count(q),' ',ToArray(l));
  foreach var x in q do Writeln(x);
  var records:=new R[1]; records[0].x:=7;
  var rs:sequence of R:=records;
  var copied:=rs.ToArray; copied[0].x:=9;
  Writeln(records[0].x,' ',copied[0].x);
  var empty:array of integer;
  s:=empty;
  Writeln(s=nil);
end.
