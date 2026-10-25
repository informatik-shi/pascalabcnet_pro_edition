type R=record n:integer; end;
begin
  var r:R; r.n:=7;
  Writeln('[',r:6,'] [',Arr(1,2):8,']');
  Writeln('[',1.5:6:2,'] [',1.5:-6:-2,'] [','🌍':4,']');
  var f:=OpenWrite('formatted-output-test.txt');
  f.Write(7:4,'|'); f.Writeln(1.5:7:3);
  f.Println(r:6,Arr(1,2):8);
  Close(f); Reset(f);
  Writeln('[',f.ReadlnString,']');
  Writeln('[',f.ReadlnString,']');
  Close(f); Erase(f);
end.
