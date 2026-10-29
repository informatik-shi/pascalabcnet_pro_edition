function Add(a,b: integer): integer;
begin
  Result := a+b;
end;

begin
  var numbers := Seq(1,2,3);
  var letters := 'АБ';
  foreach var pair in Zip(numbers,letters) do
    Writeln(pair.Item1,': ',pair.Item2);
  Writeln('Суммы: ',numbers.Zip(Seq(10,20),Add).ToArray);
  Writeln('Тройка: ',Zip(Seq(1),Seq('Я'),Seq(2.5)).First);
end.
