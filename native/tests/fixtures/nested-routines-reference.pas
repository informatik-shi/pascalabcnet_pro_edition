function Counter(start,step:integer):Func0<integer>;
var current:=start;
  function Next:integer;
  begin
    current+=step;
    Next:=current;
  end;
begin
  Result:=Next;
end;

function Fold(seed:integer):integer;
var current:=seed;
  function Step(n:integer):integer;
    function Adjust:integer;
    begin
      Inc(current);
      Adjust:=n+current;
    end;
  begin
    if n=0 then Step:=Adjust()
    else Step:=Step(n-1)+Adjust();
  end;
begin
  Result:=Step(4);
end;

function Factorial(number:integer):integer;
  function Product(n:integer):integer;
  begin
    if n<=1 then Product:=1
    else Product:=n*Product(n-1);
  end;
begin
  Result:=Product(number);
end;

begin
  var seed:=ReadInteger;
  var step:=ReadInteger;
  var count:=ReadInteger;
  var next:=Counter(seed,step);
  var alias:=next;
  var separate:=Counter(-seed,step+1);
  for var i:=1 to count do
    Writeln(next(),' ',alias(),' ',separate());
  Writeln(Fold(seed),' ',Factorial(count));
end.