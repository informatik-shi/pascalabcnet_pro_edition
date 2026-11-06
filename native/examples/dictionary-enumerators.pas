// Копии перечислителя имеют независимое положение в одном словаре.
type WordEnumerator = Dictionary<string,integer>.Enumerator;

begin
  var words := DictStrInt(('one',1),('two',2),('three',3));
  var first: WordEnumerator := words.GetEnumerator;
  Assert(first.Current.Key = nil);
  Assert(first.MoveNext);
  var copied := first;
  Assert(first.MoveNext);
  Writeln('Original: ',first.Current.Key,'=',first.Current.Value);
  Writeln('Copy: ',copied.Current.Key,'=',copied.Current.Value);
  copied.Dispose;
  // Dispose не сбрасывает положение и текущее значение.
  while copied.MoveNext do
    Writeln(copied.Current.Key,'=',copied.Current.Value);
  Assert(copied.Current.Key = nil);
  var keys := words.Keys.GetEnumerator;
  while keys.MoveNext do
    Writeln(keys.Current);
end.
