program UtilsBenchmarkSource;
uses Utils;
var count := 0;
procedure Tick := Inc(count);
begin
  var elapsed := Benchmark(Tick);
  Writeln(count, ' ', elapsed >= 0);
  elapsed := Utils.Benchmark(procedure -> begin count += 2 end, 3);
  Writeln(count, ' ', elapsed >= 0);
  elapsed := Benchmark(Tick,-2);
  Writeln(count, ' ', elapsed <= 0);
  elapsed := Benchmark(Tick,0);
  Writeln(count, ' ', (elapsed <> elapsed) or (elapsed > 1.0e300));
  var measure: function(p: procedure; n: integer): real := Utils.Benchmark;
  elapsed := measure(Tick,2);
  Writeln(count, ' ', elapsed >= 0)
end.
