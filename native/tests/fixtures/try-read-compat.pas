type R=record n:integer; end;
begin
  var n:=41;
  PrintDelimDefault:='|';
  Writeln(TryRead(n,'N'),' ',n);
  Writeln(TryRead(n),' ',n);
  var r:R; r.n:=5;
  Writeln(PABCSystem.TryRead(r.n),' ',r.n);
  Writeln(TryRead(r.n),' ',r.n);
  var u:uint64:=0;
  Writeln(TryRead(u),' ',u);
  Writeln(TryRead(u),' ',u);
  var b:boolean:=true;
  Writeln(TryRead(b),' ',b);
  Writeln(TryRead(b),' ',b);
  Writeln('[',ReadlnString,']');
  Writeln(TryRead(n),' ',n);
end.
