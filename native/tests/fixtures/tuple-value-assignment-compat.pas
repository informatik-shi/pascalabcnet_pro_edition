program TupleValueAssignment;
var trace: string := '';
var evaluations: integer := 0;
function Values: (integer,string,boolean);
begin
  Inc(evaluations);
  Result:=(10,'ten',true);
end;
function At(index: integer): integer;
begin
  trace+=IntToStr(index);
  Result:=index;
end;
begin
  var a:=0; var b:='';
  (a,b):=Values;
  Writeln(a,' ',b,' ',evaluations);

  var items:=Arr(7,8);
  (items[At(1)],items[At(0)]):=items;
  Writeln(items[0],' ',items[1],' ',trace);

  var visits:=0;
  var mapped:=Seq(100,200,300).Select(x->begin Inc(visits); Result:=x; end);
  var x,y,z: integer;
  (x,y,z):=mapped;
  Writeln(x,' ',y,' ',z,' ',visits);

  var d:=new Dictionary<integer,string>;
  d.Add(5,'five');
  (a,b):=d.First;
  Writeln(a,' ',b);
end.
