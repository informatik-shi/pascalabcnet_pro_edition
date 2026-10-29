function F(first:integer;params values:array of integer):integer;
begin Result:=first+values.Sum; end;
function Identity(params values:array of integer):array of integer;
begin Result:=values; end;
begin
  Writeln(F(7),' ',F(7,1,2),' ',F(7,Arr(3,4)));
  var a:=Arr(1,2);
  var b:=Identity(a);
  b[0]:=9;
  Writeln(a,' ',a=b,' ',Identity.Length);
end.
