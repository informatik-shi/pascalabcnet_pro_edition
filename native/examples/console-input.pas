uses Arrays;
begin
  var x,y:integer;
  Write('Два целых: ');
  Readln(x,y);
  Writeln('Сумма: ',x+y);
  var a:=new integer[3];
  Write('Три числа: ');
  ReadArray(a);
  WritelnArray(a,', ');
end.
