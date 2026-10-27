uses School;
function Make: sequence of Addr32;
begin
  var c := new CalcIP('10.0.0.1',30);
  Result := c.GenAddr;
end;
begin
  var s := Make;
  Writeln(s.Count,' ',s.ToArray[1].Value);
  foreach var a in s do foreach var b in s do
    Writeln(a.Value,' ',b.Value);
  var c := new CalcIP('128.0.0.1',2);
  foreach var a in c.GenAddr do
  begin
    Writeln(a.Value);
    break;
  end;
  var small := new CalcIP('10.0.0.1',30);
  var deferred := small.GenAddr;
  small.Network.Value := '10.0.0.10';
  foreach var a in deferred do
  begin
    Writeln(a.Value);
    small.Network.Value := '10.0.0.20';
  end;
end.
