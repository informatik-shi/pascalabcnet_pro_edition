// Native HashSet: concrete element types and source set operators.
begin
  var first := new HashSet<integer>(Arr(1, 2, 3, 3));
  var second := new HashSet<integer>(Arr(3, 4));
  Writeln(first, ' ', first.Count);
  Writeln(first + second);
  Writeln(first * second);
  Writeln(first - second);
  first += 5;
  first -= 2;
  Writeln(first, ' ', 5 in first, ' ', 2 not in first);
  Writeln(first.IsSubsetOf(Arr(1, 3, 5, 9)));
  Writeln(first.Where(x -> x > 1).ToArray);
  var copy1 := new HashSet<integer>(first);
  copy1.Add(9);
  Writeln(first, ' ', copy1);
end.
