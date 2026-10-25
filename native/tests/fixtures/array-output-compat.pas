uses Arrays;
begin
  var a:=Arr(1,2,3);
  WriteArray(a); Writeln('|');
  a.Writeln(',');
  Arrays.WritelnArray(Arr(true,false),';');
  var n:array of integer:=nil;
  WriteArray(n); WritelnArray(n);
  var s:string:=nil;
  WritelnArray(Arr('Анна',s,'Вера'),'|');
  WritelnArray(Arr(MaxUInt64));
end.
