type R=record u:uint64; w:longword; end;
function Next(u:uint64):uint64;
begin Result:=u+1; end;
begin
  Writeln(MaxByte,' ',MaxShortInt,' ',MaxSmallInt,' ',MaxWord,' ',MaxLongWord,' ',MaxUInt64);
  var r:R; r.u:=MaxUInt64; r.w:=MaxLongWord;
  Writeln(r.w+1,' ',r.u div 2,' ',r.u mod 2);
  Writeln(Sqr(MaxWord),' ',Sqr(MaxLongWord));
  Writeln(Min(r.u,1),' ',Next(MaxInt64));
  for var u:=MaxUInt64-1 to MaxUInt64 do Writeln(u);
  var a:=Arr(1,MaxUInt64);
  Writeln(a[0],' ',a[1]);
  var flag:=false; Inc(flag); Writeln(flag); Dec(flag); Writeln(flag);
  Writeln(Succ(true),' ',Pred(false),' ',false<true);
  for var b:=false to true do Write(Ord(b));
  for var b:=true downto false do Write(Ord(b)); Writeln;
  Writeln(NewLine.Length>=1,' ',Ord(END_OF_LINE_SYMBOL),' ',Pos('№',AllDelimiters)>0);
  var f:=OpenWrite('native-unsigned-integers.txt');
  f.Writeln(r.w,' ',r.u); f.Close; f.Reset;
  var w:longword; var u:uint64; Read(f,w,u); f.Close; f.Erase;
  Writeln(w,' ',u);
end.
