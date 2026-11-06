// Интерфейс разделяет состояние обхода между своими копиями.
type IntIterator = IEnumerator<integer>;

function TakeNext(e: IntIterator): integer;
begin
  Assert(e.MoveNext);
  Result := e.Current;
end;

begin
  var source: IEnumerable<integer> := Arr(10,20,30);
  var first := source.GetEnumerator;
  var shared := first;
  Writeln(TakeNext(first),' ',TakeNext(shared));
  Writeln(TakeNext(first));

  var words := DictStrInt(('one',1),('two',2));
  var values: IntIterator := words.Values.GetEnumerator;
  var alias := values;
  Writeln(TakeNext(values),' ',TakeNext(alias));
  values.Reset;
  while alias.MoveNext do
    Writeln(alias.Current);

  var squares := Seq(2,3).Select(x->x*x).GetEnumerator;
  Assert(squares.MoveNext);
  Writeln(squares.Current);
  squares.Dispose;
  Writeln(squares.MoveNext);
end.
