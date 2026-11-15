program CountedLoop;
var calls: integer := 0;
function Iterations: integer;
begin
  Inc(calls);
  Result:=5;
end;

procedure Nested;
begin
  loop word(2) do
    loop byte(3) do Write('x');
  Writeln;
end;

procedure Signed(value: int64);
begin
  var completed:=0;
  loop value do
  begin
    Inc(completed);
    if completed=3 then break;
  end;
  Writeln(completed);
end;
procedure Unsigned(value: uint64);
begin
  var completed:=0;
  loop value do
  begin
    Inc(completed);
    if completed=3 then break;
  end;
  Writeln(completed);
end;
begin
  var visited:=0;
  loop Iterations do
  begin
    Inc(visited);
    if visited=2 then continue;
    Write(visited);
    if visited=4 then break;
  end;
  Writeln(' ',calls,' ',visited);
  var limit:=3;
  loop limit do
  begin
    limit:=0;
    Write('y');
  end;
  Writeln;
  loop -1 do Writeln('unexpected negative iteration');
  loop 0 do Writeln('unexpected zero iteration');
  Nested;
  var launch: procedure := ()->begin
    loop 2 do Write('z');
  end;
  launch;
  Writeln;
  Signed(2147483648);
  Signed(4294967297);
  Signed(-4294967295);
  Signed(-2147483649);
  Unsigned(4294967297);
  Unsigned(uint64(9223372036854775807)+1);
  Unsigned(uint64(9223372036854775807)*2+1);
end.
