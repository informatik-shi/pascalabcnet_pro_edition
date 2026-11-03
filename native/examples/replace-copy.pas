begin
  var a := Arr(1,2,1);
  Writeln('Замена: ', a.Replace(1,7));
  var b := Copy(a);
  b[0] := 9;
  Writeln('Массивы: ', a, ' ', b);
  var l := new List<string>;
  l.Add('abc'); l.Add('abc');
  l.Replace('abc','xyz');
  var other := Copy(l);
  other[0] := 'new';
  Writeln('Списки: ', l, ' ', other);
  var m := Matr(1,2,3,4);
  var n := m.Copy;
  n[0,0] := 8;
  Writeln('Матрицы: ', m, ' ', n);
end.
