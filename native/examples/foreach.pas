begin
  var numbers:=Arr(3,1,4,1,5);
  var total:=0;
  foreach var n in numbers do total:=total+n;
  Writeln('Сумма: ',total);

  var names:=new List<string>;
  names.Add('Анна'); names.Add('Борис');
  Write('Имена: ');
  foreach var name in names do Write(name,' ');
  Writeln;

  Write('Нечётные: ');
  foreach var n in numbers do
  begin
    if n mod 2=0 then continue;
    Write(n,' ');
  end;
  Writeln;
end.
