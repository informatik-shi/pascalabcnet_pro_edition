begin
  var p:=1;
  var s:='  -12| 3.5  🌍';
  Writeln(ReadIntegerFromString(s,p),' ',p);
  Writeln('[',ReadWordFromString(s,p),'] ',p);
  Writeln(ReadRealFromString(s,p),' ',p);
  Writeln(ReadWordFromString(s,p),' ',p);
  Writeln(StringIsEmpty(s,p),' ',p);
  p:=1; var n:=7;
  Writeln(TryReadIntegerFromString('bad 42',p,n),' ',p,' ',n);
  p:=5;
  Writeln(PABCSystem.TryReadIntegerFromString('bad 42',p,n),' ',p,' ',n);
  p:=1; var r:real:=7;
  Writeln(TryReadRealFromString('bad',p,r),' ',p,' ',r);
  p:=1;
  Writeln(TryReadIntegerFromString('42',p,p),' ',p);
end.
