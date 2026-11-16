program TupleAssignmentOrder;
var trace: string := '';
function R(value: integer): integer;
begin
  trace+= 'R'+IntToStr(value);
  Result:=value;
end;
function L(value: integer): integer;
begin
  trace+= 'L'+IntToStr(value);
  Result:=value;
end;
procedure ViaVar(var x,y: integer);
begin
  (x,y):=(y+1,x+2);
end;
begin
  var a:=1; var b:=2; var c:=3;
  (a,b,c):=(b,a+b,a);
  Writeln(a,' ',b,' ',c);
  (a,b):=(R(1),R(2),R(3));
  Writeln(a,' ',b,' ',trace);
  trace:='';
  var values:=Arr(10,20);
  (values[L(0)],values[L(1)]):=(R(7),R(8));
  Writeln(values[0],' ',values[1],' ',trace);
  trace:='';
  (values[L(1)],values[L(0)]):=(values[0],values[1]);
  Writeln(values[0],' ',values[1],' ',trace);
  (values[1],values[0]):=values;
  Writeln(values[0],' ',values[1]);
  trace:='';
  (a,a):=(R(1),R(2),R(3));
  Writeln(a,' ',trace);
  a:=10;
  ViaVar(a,a);
  Writeln(a);
  var p: ^integer;
  New(p);
  p^:=6;
  a:=4;
  (p^,a):=(a,p^);
  Writeln(p^,' ',a);
  Dispose(p);
end.
