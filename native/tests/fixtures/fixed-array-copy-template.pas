var calls:integer;
function Bump:integer;
begin Inc(calls);Write('i');Result:=calls;end;
type
  Inner=array[0..1] of byte;
  Item=record n:integer:=Bump;inner:Inner;end;
  Items=array[@LOW@..@HIGH@] of Item;
procedure ByValue(a:Items);
begin Writeln('V',calls);end;
procedure ByConst(const a:Items);
begin Writeln('C',calls);end;
begin
  var mode:=ReadInteger;
  var a:Items;
  for var i:=Low(a) to High(a) do begin a[i].n:=i*11+13;a[i].inner[0]:=byte(i-Low(a)+7);end;
  Writeln('A',calls);
  var b:=a;
  b[Low(b)].n:=999;b[Low(b)].inner[0]:=88;
  Writeln('B',calls,':',a[Low(a)].n,',',a[Low(a)].inner[0]);
  ByValue(a);ByConst(a);
  var f:file of Items;
  Writeln('F',calls);
  Rewrite(f,'native-fixed-reference.tmp');Write(f,a);Reset(f);
  var r:Items;
  Read(f,r);Writeln('R',calls,':',r[Low(r)].n,',',r[Low(r)].inner[0]);
  if mode=1 then Read(f,r);
  Close(f);
end.
