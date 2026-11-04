uses System.Text;

begin
  var name:='native-streaming-lines.tmp';
  WriteAllLines(name,Arr('Первая строка','','Последняя строка'),Encoding.Unicode);
  var lines:sequence of string:=ReadLines(name,Encoding.ASCII);
  Writeln('Первые две: ',lines.Take(2).ToArray);
  Writeln('Повторный обход: ',lines.Where(line->line<>'').ToArray);
  foreach var line in lines do
  begin
    Writeln('Остановка на строке: ',line);
    break;
  end;
  Writeln('После break файл закрыт: ',DeleteFile(name));
end.
