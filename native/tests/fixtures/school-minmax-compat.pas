uses School;
begin
  Writeln(MinMax(Arr(4,-7,9,2)));
  Writeln(School.MinMax(Arr(int64(-9223372036854775808),MaxInt64)));
  var a := new List<real>;
  a.Add(1.5);
  a.Add(-2.25);
  Writeln(a.MinMax);
  Writeln(MinMax(new integer[0]));
  var r := MinMax(new real[0]);
  Writeln(r.Item1>1e308,' ',r.Item2< -1e308);
end.
