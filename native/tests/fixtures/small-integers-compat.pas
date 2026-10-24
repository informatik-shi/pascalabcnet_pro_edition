type R=record b:byte; s:shortint; n:smallint; end;
procedure Advance(var b:byte);
begin Inc(b); end;
begin
  var r:R;
  Writeln(r.b,' ',r.s,' ',r.n);
  r.b:=254; r.s:=-128; r.n:=-32768;
  Advance(r.b);
  Writeln(r.b,' ',r.s,' ',r.n);
  Writeln(r.b+1,' ',r.s-1,' ',r.n-1);
  Writeln(Sqr(byte(255)),' ',Sqr(shortint(-128)),' ',Sqr(smallint(-32768)));
  Writeln(Abs(shortint(-127)),' ',Sign(shortint(-1)),' ',Max(byte(1),byte(255)));
  for var b:byte:=254 to 255 do Writeln(b);
  for var s:shortint:=-127 downto -128 do Writeln(s);
  for var n:smallint:=32766 to 32767 do Writeln(n);
  var a:=Arr(byte(1),shortint(-2));
  foreach var n:integer in a do Write(n,'|'); Writeln;
  var l:=new List<byte>; l.Add(255); l.Add(0); Sort(l);
  Writeln(l[0],' ',l[1]);
  var f:=OpenWrite('native-small-integers.txt'); f.Writeln(r.b,' ',r.s,' ',r.n); f.Close;
  f.Reset; var b:byte; var s:shortint; var n:smallint; Read(f,b,s,n); f.Close; f.Erase;
  Writeln(b,' ',s,' ',n);
end.
