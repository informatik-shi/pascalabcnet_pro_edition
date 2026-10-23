type Point=record x:integer; end;
var calls:integer;
function Source:array of integer;
begin Inc(calls); Result:=Arr(1,2,3); end;
function Sum(a:array of integer):integer;
begin foreach var x in a do Result:=Result+x; end;
begin
  foreach var x in Source do Write(x);
  Writeln(' ',calls);
  var x:=99;
  foreach x in Arr(4,5) do Write(x);
  foreach x in new integer[0] do Write('bad');
  Writeln(' ',x);
  foreach n:int64 in Arr(6,7) do Write(n,' ');
  Writeln('|');
  var l:=new List<string>;
  l.Add('one'); l.Add('two');
  foreach var s in l do Write(s,' ');
  Writeln('|');
  var a:=Arr(1,2,3);
  foreach var value in a do
  begin
    Write(value); if value=1 then a[1]:=9;
  end;
  Writeln;
  foreach var value in a do begin Write(value); SetLength(a,0); end;
  Writeln(' ',Length(a));
  var p:Point; p.x:=3;
  var points:=Arr(p,p);
  foreach var item in points do begin Write(item.x,' '); points[1].x:=7; end;
  Writeln('|');
  var row:=Arr(1);
  foreach var item in Arr(row) do item[0]:=8;
  Writeln(row[0]);
  var total:=0;
  foreach var i in Arr(1,2,3) do
  begin
    foreach var j in Arr(1,2,3) do
    begin
      if j=2 then break;
      total:=total+i;
    end;
  end;
  Writeln(total,' ',Sum(Arr(4,5)));
end.
