uses Arrays;
type
  TPoint = record x,y: integer; end;
  TNumbers = array of integer;
var visits: integer;
var text: string;
function PositionAfterChange: integer;
begin
  text := 'XYZ';
  Result := 2;
end;
function Factorial(n: integer): int64;
begin
  if n <= 1 then Exit(1);
  Result := n * Factorial(n-1);
end;
procedure Bump(var x: integer; step: integer);
begin
  Inc(visits);
  Inc(x,step);
end;
procedure Replace(var p: TPoint; var field: integer);
var q: TPoint;
begin
  q.x := 7;
  p := q;
  field := 9;
end;
function LabelText: string;
begin Result := 'Привет'; end;
begin
  var a: TNumbers := Arr(3,1,2);
  var alias := a;
  a[0] := 4;
  Sort(a);
  Writeln(a[0],a[1],a[2], ' ',alias[0]);
  var copied := Copy(a);
  copied[0] := 9;
  Writeln(a[0], ' ', copied[0]);
  SetLength(alias,4);
  Writeln(Length(a),' ',Length(alias),' ',alias[3],' ',High(a));
  FillByValue(copied,5);
  Writeln(copied[0],copied[1],copied[2]);
  var p: TPoint;
  p.x := 3;
  var q := p;
  q.x := 8;
  Writeln(p.x,' ',q.x);
  Replace(p,p.x);
  Writeln(p.x,' ',p.y);
  Bump(a[0],5);
  Writeln(a[0],' ',visits,' ',Factorial(10));
  Writeln(LabelText);
  var points := new TPoint[2];
  points[0] := p;
  points[0].x := 4;
  Writeln(p.x,' ',points[0].x,' ',points[1].x);
  var xs := new List<integer>;
  xs.Add(7); xs.Add(2); xs.Add(5);
  SortDescending(xs);
  Writeln(xs.Count,' ',xs[0],xs[1],xs[2],' ',xs.Contains(2));
  xs.RemoveAt(1); Writeln(xs.Count,' ',xs[1]);
  xs.Clear(); Writeln(xs.Count);
  WriteAllText('native-text-test.txt','текст');
  Writeln(FileExists('native-text-test.txt'),' ',ReadAllText('native-text-test.txt'));
  WriteAllLines('native-lines-test.txt',Arr('alpha','beta'));
  var lines := ReadAllLines('native-lines-test.txt');
  Writeln(Length(lines),' ',lines[0],' ',lines[1]);
  text := 'abc';
  Insert('!',text,PositionAfterChange());
  Writeln(text);
end.
