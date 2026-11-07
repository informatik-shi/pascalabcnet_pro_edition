// Копии записей независимы, копии интерфейса разделяют состояние.
type StringCursor = List<string>.Enumerator;

begin
  var words := Lst('one','two','three');
  var original: StringCursor := words.GetEnumerator;
  Assert(original.MoveNext);
  var copied := original;
  Assert(original.MoveNext);
  Writeln(original.Current,' ',copied.Current);

  var shared: IEnumerator<string> := original;
  var alias := shared;
  Assert(alias.MoveNext);
  Writeln(shared.Current,' ',original.Current);
  shared.Reset;
  Assert(alias.MoveNext);
  Writeln(alias.Current);
  Assert(shared.GetType = typeof(StringCursor));

  var colors := HSet('red','blue');
  var cursor: HashSet<string>.Enumerator := colors.GetEnumerator;
  while cursor.MoveNext do
    Writeln(cursor.Current);
end.
