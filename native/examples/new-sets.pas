// NewSet: assignment copies the set; procedure arguments share its backing store.
procedure AddThroughArgument(items: NewSet<integer>);
begin
  items += 7;
end;

begin
  var first := SetOf(1, 2, 3, 3);
  var second: set of integer := Arr(3, 4);
  Writeln(first, ' ', first.Count);
  Writeln(first + second, ' ', first * second, ' ', first - second);
  var independent := first;
  independent += 5;
  Writeln(first, ' ', independent);
  AddThroughArgument(first);
  Writeln(first, ' ', 7 in first, ' ', 9 not in first);
  first *= SetOf(2, 7, 8);
  first.AddRange(Seq(9, 10));
  Writeln(first);
  Writeln(first.Where(x -> x > 7).ToArray);
  var hash: HashSet<integer> := first;
  hash.Add(11);
  Writeln(first, ' ', hash);
end.
