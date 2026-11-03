begin
  var a := Arr(-3,0,4);
  Writeln('Модули: ', a.Select(x -> (if x<0 then -x else x)).ToArray);
  var m := MatrByRow(Seq(Seq(1,2),Seq(3)));
  Writeln('Ширина: ', if m.RowCount=0 then 0 else m.Row(0).Length);
  Writeln('Выбор: ', m.RowCount>0 ? 'есть строки' : 'пусто');
  Writeln('Неисполняемая ветвь: ', if True then 7 else 1 div 0);
end.
