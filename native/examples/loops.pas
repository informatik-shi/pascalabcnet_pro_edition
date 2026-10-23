begin
  var sum:=0;
  for var i:=1 to 10 do sum:=sum+i;
  Writeln('Сумма: ',sum);

  Write('Обратный отсчёт: ');
  for var n:=3 downto 1 do Write(n,' ');
  Writeln;

  var count:=0;
  repeat
    Inc(count);
  until count=3;
  Writeln('Повторений: ',count);

  Write('Нечётные: ');
  for var n:=1 to 8 do
  begin
    if n mod 2=0 then continue;
    if n>5 then break;
    Write(n,' ');
  end;
  Writeln;
end.
