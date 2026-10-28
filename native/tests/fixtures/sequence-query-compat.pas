uses School;
begin
  var a := Arr(4, 7, 9);
  var s: sequence of integer := a;
  Writeln(s.Any, ' ', Any(new integer[0]));
  Writeln(s.First, ' ', Last(a), ' ', PABCSystem.ElementAt(a, 1));
  Writeln(s.Contains(7), ' ', Contains(a, 8));
  Writeln((new integer[0]).FirstOrDefault, ' ', a.ElementAtOrDefault(-1), ' ', a.ElementAtOrDefault(3));
  Writeln((new string[0]).LastOrDefault = nil);
  var list := a.ToList();
  Writeln(list.First, ' ', list.Last, ' ', list.LastOrDefault);
  var ip := new CalcIP('10.0.0.1', 30);
  Writeln(ip.GenAddr.Any, ' ', ip.GenAddr.First.Value, ' ', ip.GenAddr.Last.Value);
  Writeln(ip.GenAddr.ElementAtOrDefault(2) = nil);
end.
