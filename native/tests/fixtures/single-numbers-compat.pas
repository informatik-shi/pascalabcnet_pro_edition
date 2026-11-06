// IEEE 754 single: the type and every operation are checked before execution.
procedure AddQuarter(var value: single);
begin
  value := value + single(0.25);
end;

begin
  var value: single := 16777216;
  Writeln(value + 1, ' ', TypeName(value + 1));
  Writeln(single.Parse('1.000000059604644775390626'));
  var values := Arr(single(16777216), single(1), single(-16777216));
  Writeln(values.Sum, ' ', values.Average);
  AddQuarter(value);
  Writeln(value);
  var parsed: single;
  Writeln(TryStrToSingle('1e-45', parsed), ' ', parsed);
  Writeln(MaxSingle, ' ', MinSingle);
end.
