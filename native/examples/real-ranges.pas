begin
  var s:=Range(0,1,0.1);
  Writeln('Точек: ',s.Count);
  Writeln('Конец точно 1: ',s.Last=1);
  Writeln('Четверти: ',Range(0,1,0.25).ToArray);
  Writeln('Обратно: ',Range(1,0,-0.25).ToList);
  Writeln('Шаг 0.3: ',Range(0,1,0.3).Count,' точки');
end.
