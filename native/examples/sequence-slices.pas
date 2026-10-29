begin
  var a:=Arr(1,2,3,4,5);
  Writeln('Первые: ',a.Take(2).ToArray);
  Writeln('Пропустить: ',a.Skip(2).ToList);
  Writeln('Последние: ',a.TakeLast(2).ToArray);
  Writeln('Без последних: ',a.SkipLast(2).ToArray);
  Writeln('Без одного последнего: ',a.SkipLast.ToArray);
  Writeln('Ранняя остановка: ',Range(MaxInt,MaxInt,1).Take(1).Single);
end.
