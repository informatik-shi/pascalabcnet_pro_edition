procedure Add(const first,second:integer;out total:integer);
begin total:=first+second; end;
procedure Bump(out value:integer);
begin Inc(value); end;
begin
  var total:=0;
  Add(20,22,total);
  Writeln('Сумма: ', total);
  Bump(total);
  Writeln('После изменения: ', total);
end.
