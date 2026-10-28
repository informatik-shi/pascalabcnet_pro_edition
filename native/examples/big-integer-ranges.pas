begin
  var start:=1000000000000000000000000000000bi;
  Writeln('Большие числа: ',Range(start,start+6,2).ToArray);
  Writeln('Обратный порядок: ',Range(7bi,1bi,-2bi).ToList);
  var huge:=Range(0bi,1bi shl 100);
  Writeln('Первый огромного диапазона: ',huge.First);
  foreach var x in huge do
  begin
    Writeln('Ранний выход: ',x);
    break;
  end;
end.
