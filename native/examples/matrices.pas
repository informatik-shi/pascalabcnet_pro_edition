uses Arrays;
begin
  SetRandomSeed(1);
  var a:=CreateRandomIntegerMatrix(2,3);
  Writeln('Матрица:');
  WriteMatrix(a,4);
  var b:=a.Copy;
  b[0,0]:=-1;
  SetLength(b,3,2);
  Writeln('Размер копии: ',b.RowCount,'x',b.ColCount);
  Writeln('Исходный элемент: ',a[0,0]);
end.
