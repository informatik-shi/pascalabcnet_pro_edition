begin
  Writeln('По значениям: ', Matr(2,3,1,2,3,4,5,6));
  var rows := Arr(Arr(1,2,3),Arr(4,5));
  Writeln('По строкам: ', MatrByRow(rows));
  Writeln('По столбцам: ', MatrByCol(rows));
  Writeln('Из последовательности: ', MatrByCol(2,3,Seq(1,2,3,4)));
  Writeln('Вложенные последовательности: ', MatrByRow(Seq(Seq(7,8),Seq(9))));
  var text := MatrByRow(2,2,Seq('abc'));
  Writeln('Значение по умолчанию: ', text[1,1]=nil);
end.
