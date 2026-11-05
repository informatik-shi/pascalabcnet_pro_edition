begin
  var s := new HashSet<integer>(Arr(1,2,3,4));
  s.Remove(2);
  var a := Arr(9,9,9,9,9);
  s.CopyTo(a,1,4);
  Writeln(a);
  foreach var x in s do
  begin
    Write(x);
    if x=3 then s.TrimExcess;
  end;
  Writeln;
  s.Add(5);
  Writeln(s.RemoveWhere(x -> x mod 2=0),' ',s);
  var actual := 99;
  Writeln(s.TryGetValue(3,actual),' ',actual);
  Writeln(s.TryGetValue(8,actual),' ',actual);
end.
