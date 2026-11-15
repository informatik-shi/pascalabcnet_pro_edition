var Calls := 0;
const OuterLimit = 7;

function NextValue: integer;
begin
  Inc(Calls);
  Result := Calls;
end;

procedure Run;
const InnerLimit = OuterLimit + 1;

procedure Show(n: integer := InnerLimit; s: string := 'abc');
begin
  Writeln(n, ':', s);
end;

procedure CountDefaults(a: integer := NextValue; b: integer := NextValue);
begin
  Writeln(a, ' ', b, ' ', Calls);
end;

function Countdown(n: integer; step: integer := 2): integer;
begin
  if n <= 0 then exit(1);
  Result := n * Countdown(n-step);
end;

function Total(n: integer := 4): integer := Countdown(n);

procedure CheckNil(a: array of integer := nil; s: string := nil);
begin
  Writeln(a = nil, ' ', s = nil);
end;

begin
  Show;
  Show();
  Show(10);
  Show(11, 'text');
  CountDefaults;
  CountDefaults(9);
  CountDefaults(8,7);
  Writeln(Total, ' ', Total(), ' ', Countdown(5, 1));
  CheckNil;
end;

begin
  Run;
end.
