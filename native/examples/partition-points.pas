begin
  Writeln('Разбиение на 4 части: ',PartitionPoints(0,1,4).ToArray);
  Writeln('Пять точек: ',Linspace(0,1,5).ToArray);
  Writeln('Обратный порядок: ',Linspace(1,0,5).ToList);
  Writeln('Первый большого разбиения: ',PartitionPoints(0,1,MaxInt).First);
end.
