var calls:integer;
function Start:integer;
begin Inc(calls);Write('init ');Result:=7;end;
type Shared=class
public
  static Value:integer:=Start;
  static Values:array[1..3] of integer:=(1,2,3);
  Local:integer;
  constructor Create(n:integer);
  begin Local:=n;end;
end;
begin
  Write('before ');
  Writeln(Shared.Value,' ',Shared.Value,' ',calls);
  var a:=new Shared(10);var b:=new Shared(20);
  Shared.Values[1]:=99;
  Writeln(Shared.Values[1],' ',a.Local,' ',b.Local);
end.
