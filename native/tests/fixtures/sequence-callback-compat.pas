var calls:integer;
function Twice(value:integer):integer;
begin Inc(calls); Result:=value*2; end;
function MultipleOfFour(value:integer):boolean;
begin Result:=value mod 4=0; end;
begin
  var s:=Range(1,5).Select(Twice).Where(MultipleOfFour);
  Writeln(calls);
  Writeln(s.ToArray,' ',calls);
  Writeln(s.First,' ',calls);
end.
