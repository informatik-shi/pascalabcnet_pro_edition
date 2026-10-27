uses School;
type R = record x: integer; end;
begin
  var a := Arr(1, 2);
  var list := a.ToList();
  list[0] := 9;
  list.Add(3);
  Writeln(a, ' ', list);
  var s: sequence of integer := list;
  var copied := s.ToList;
  list[1] := 8;
  Writeln(copied, ' ', list);
  Writeln(list.ToArray());
  var records := new R[1];
  records[0].x := 7;
  var rl := PABCSystem.ToList(records);
  rl[0].x := 9;
  Writeln(records[0].x, ' ', rl[0].x);
  var ip := new CalcIP('10.0.0.1', 30);
  var hosts := ip.GenAddr.ToList();
  Writeln(hosts[0].Value, ' ', hosts[1].Value);
  Writeln(ToList(new integer[0]).Count);
end.
