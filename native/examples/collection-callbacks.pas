begin
  var a := Arr(1,2,3);
  Writeln('Преобразование: ', a.Transform((x,i) -> x+i));
  a.Fill(i -> i*2);
  Writeln('Заполнение: ', a);
  Writeln('Новый тип: ', a.ConvertAll(x -> IntToStr(x)));
  var m := MatrFill(2,2,0);
  m.Fill((r,c) -> r*10+c);
  m.Transform((x,r,c) -> x+r+c);
  Writeln('Матрица: ', m);
  var total := 0;
  m.ForEach(x -> total := total+x);
  &ForEach(Seq(1,2),(x,i) -> total := total+x+i);
  Writeln('Действия: ', total);
  var l := new List<integer>;
  l.Add(2); l.Add(3);
  Writeln('Список: ', l.Transform(x -> x*2).Fill(i -> i+10).ConvertAll(x -> IntToStr(x)));
end.
