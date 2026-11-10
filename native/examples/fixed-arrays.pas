type
  Row = array[-2..1] of integer;
  Matrix = array[1..2,3..4] of integer;
  Pair = record a:Row; b:integer;end;
procedure Change(var a:Row);
begin a[-2]:=17;end;
procedure CopyValue(a:Row);
begin a[-2]:=99;Writeln(a[-2]);end;
begin
  var a:Row;
  a[-2]:=5;a[1]:=8;
  var b:=a;
  b[-2]:=10;
  Writeln(a[-2],' ',b[-2],' ',Length(a),' ',Low(a),' ',High(a));
  CopyValue(a);Change(a);Writeln(a[-2]);
  var m:Matrix;
  m[1,3]:=7;m[2,4]:=9;Writeln(m[1,3],' ',m[2,4]);
  var r:Pair;r.a[1]:=42;var s:=r;s.a[1]:=31;Writeln(r.a[1],' ',s.a[1]);
  var f:file of Matrix;
  Rewrite(f,'native-fixed-array.tmp');Write(f,m);Reset(f);var n:Matrix;Read(f,n);Writeln(n[1,3],' ',n[2,4],' ',FileSize(f));Close(f);Erase(f);
end.
