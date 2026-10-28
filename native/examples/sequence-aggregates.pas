begin
  var a:=Arr(1,2,3,4);
  Writeln('Сумма: ',a.Sum);
  Writeln('Среднее: ',a.Average);
  Writeln('Произведение: ',a.Product);
  Writeln('Частичные суммы: ',a.PartialSum.ToArray);
  Writeln('BigInteger: ',Arr(2bi,3bi,4bi).Product);
  Writeln('Среднее пустой BigInteger: ',(new BigInteger[0]).Average);
end.
