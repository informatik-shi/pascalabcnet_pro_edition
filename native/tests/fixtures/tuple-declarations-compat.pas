program TupleDeclarationsSource;
var evaluations: integer := 0;
    (leftGlobal,rightGlobal) := (7,8);
function Source: (integer,string,boolean);
begin
  Inc(evaluations);
  Result:=(10,'ten',true);
end;
begin
  Writeln(leftGlobal,' ',rightGlobal);
  var (number,labelText) := Source;
  labelText := labelText + '!';
  Writeln(number,' ',labelText,' ',evaluations);

  var d:=new Dictionary<integer,string>;
  d.Add(5,'five');
  var (key,value) := d.First;
  Writeln(key,' ',value);

  var (a,b) := Arr(1,2,3);
  Writeln(a,' ',b);

  var visits:=0;
  var mapped:=Seq(10,20,30).Select(x->begin Inc(visits); Result:=x; end);
  var (first,second,third) := mapped;
  Writeln(first,' ',second,' ',third,' ',visits);

  var rows: array of array of integer;
  SetLength(rows,1);
  rows[0]:=Arr(5,6,7);
  foreach var (leftItem,rightItem) in rows index i do Writeln(i,' ',leftItem,' ',rightItem);

  var wrapped: array of sequence of integer;
  SetLength(wrapped,1);
  wrapped[0]:=mapped;
  foreach var (x,y) in wrapped do Writeln(x,' ',y,' ',visits);
end.
