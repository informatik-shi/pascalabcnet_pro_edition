program ForeachTupleSource;
type Callback = ()->integer;
begin
  foreach var (number, labelText) in Arr((1,'one'),(2,'two')) index i do
    Writeln(i,' ',number,' ',labelText);

  foreach var (first, second) in Arr((1,2,3)) do
  begin
    first += 100;
    Writeln(first,' ',second);
  end;

  foreach var (only) in Arr((7,8)) do Writeln(only);

  var d:=new Dictionary<integer,string>;
  d.Add(5,'five');
  d.Add(9,'nine');
  foreach var (key, textValue) in d index j do
    Writeln(j,' ',key,' ',textValue);

  var callbacks:=new List<Callback>;
  foreach var (a,b) in Arr((1,2),(3,4)) do
    callbacks.Add(()->a+b);
  Writeln(callbacks[0](),' ',callbacks[1]());
end.
