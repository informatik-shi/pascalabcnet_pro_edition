begin
  var s:string;
  Str(MaxUInt64,s); Writeln(s);
  PABCSystem.Str(Pi:8:3,s); Writeln('[',s,']');
  Str(nil,s); Writeln(s=nil);
  var n:=41; var e:=9;
  Val('bad',n,e); Writeln(n,' ',e);
  Val('42',n,e); Writeln(n,' ',e);
  var u:uint64:=7;
  Val('18446744073709551615',u,e); Writeln(u,' ',e);
  Val('18446744073709551616',u,e); Writeln(u,' ',e);
  var r:real:=7;
  Writeln(TryStrToReal('1,5',r),' ',r);
  Writeln('1.5'.TryToReal(r),' ',r);
  Writeln('bad'.TryToInteger(n),' ',n);
  Writeln(PABCSystem.TryStrToInt(' 5 ',n),' ',n);
end.
