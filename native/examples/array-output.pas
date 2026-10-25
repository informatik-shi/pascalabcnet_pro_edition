uses Arrays;
begin
  var numbers:=Arr(10,20,30);
  Write('Числа: ');
  WritelnArray(numbers,', ');
  var names:=Arr('Анна','Борис','Вера');
  Write('Имена: ');
  names.Writeln(' / ');
  var empty:=new integer[0];
  Write('Пустой: [');
  empty.Write;
  Writeln(']');
end.
