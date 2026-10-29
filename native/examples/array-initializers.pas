// Типы и размеры инициализаторов проверяются до исполнения.
type
  Point = record
    x,y: integer;
  end;

function MakePoint(x,y: integer): Point;
begin
  Result.x := x;
  Result.y := y;
end;

begin
  var numbers := new integer[3](10,20,30);
  Writeln('Числа: ',numbers);
  var matrix := new integer[2,2]((1,2),(3,4));
  Writeln('Матрица: ',matrix[0,1],' ',matrix[1,0]);
  var points := new Point[2](MakePoint(1,2),MakePoint(3,4));
  Writeln('Точки: ',points[0].x,' ',points[1].y);
end.
