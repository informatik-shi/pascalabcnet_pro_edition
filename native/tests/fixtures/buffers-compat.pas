uses Collections;
type Point=record x:integer; end;
begin
  var d:=new DynArray<integer>(3);
  Writeln(d.Count,' ',d.Capacity,' ',MIN_CAP,' ',INC_CAP_FACTOR);
  d[0]:=1; d.Elem[1]:=2; d[2]:=3;
  d.Resize(1); d.Count:=3;
  Writeln('[',d.ToString,']');
  d.Capacity:=12; d.Reserve(4); d.Reserve(-1); d.Count:=5;
  d.Insert(1,9); d.Remove(2);
  Writeln(d.Count,' ',d.Capacity,' ',d.Find(9),' ',d.Find(2));
  d.Print; Write('|'); d.Println;
  d.Resize(13); Writeln(d.Capacity,' ',d[12]);
  d.Count:=0; d.Count:=2; Writeln(d[0],' ',d[1]);
  var s:=new SimpleSet<integer>;
  s.Add(3); s.Add(1); s.Add(3); s.Add(2);
  Writeln('[',s.ToString,'] ',s.Contains(3),' ',s.Contains(9));
  s.Remove(9); s.Remove(1); s.Add(1);
  s.Print; Write('|'); s.Println;
  var p:Point; p.x:=7;
  var records:=new DynArray<Point>(1); records[0]:=p;
  p.x:=9; var copied:=records[0]; copied.x:=11;
  copied:=records[0]; Writeln(copied.x,' ',p.x);
  var points:=new SimpleSet<Point>; points.Add(p);
  Writeln(points.Contains(p)); points.Remove(p); Writeln(points.Contains(p));
  var a:=Arr(1); var arrays:=new SimpleSet<array of integer>;
  arrays.Add(a); arrays.Add(a); Writeln(arrays.Contains(a),' ',arrays.Contains(Arr(1)));
  arrays.Remove(a); Writeln(arrays.Contains(a));
end.
