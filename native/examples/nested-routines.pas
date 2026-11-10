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

procedure Change;
var number:=10;
  procedure Add(var target:integer;amount:integer);
  begin
    target+=amount;
    number+=1;
  end;
begin
  Add(number,3);
  Writeln('Изменено: ',number);
end;

begin
  var next:=Counter(10,2);
  Writeln(next(),' ',next(),' ',next());
  Writeln('Вложенная рекурсия: ',Fold(3));
  Change;
end.
