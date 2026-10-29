procedure Add(const a,b:integer;out total:integer);
begin total:=a+b; end;
procedure Bump(out value:integer);
begin Inc(value); end;
function Twice(const value:integer):integer;
begin Result:=value*2; end;
begin
  var total:=10;
  Add(2,3,total);
  Writeln(total);
  Bump(total);
  Writeln(total);
  Writeln(Seq(1,2,3).Select(Twice).ToArray);
end.
