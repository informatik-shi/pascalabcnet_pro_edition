begin
  var s:='12 3.5 word'; var p:=0;
  Writeln(s.ReadInteger(p),' ',p);
  Writeln(s.ReadReal(p),' ',p);
  Writeln(s.ReadWord(p),' ',p);
  Writeln('42'.ToInteger,' ','1.5'.ToReal,' ','bad'.ToInteger(7),' ','bad'.ToReal(8.5));
  Writeln('42'.IsInteger,' ','bad'.IsReal);
  var ns:string:=nil;
  Writeln(ns.IsReal,' ',ns.ToReal(9));
  Writeln(ToReal('1,234.5','.'));
end.
