function SumValues(params values:array of integer):integer;
begin Result:=values.Sum; end;
begin
  Assert(SumValues(1,2,3)=6,'Сумма трёх чисел');
  Assert(not(MaxInt)=(-MaxInt-1),'Побитовое отрицание');
  Writeln('Проверки выполнены');
end.
