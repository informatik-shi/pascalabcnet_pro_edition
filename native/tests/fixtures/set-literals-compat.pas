// Set literals preserve scalar-first insertion and source empty-set rules.
begin
  var first := [1..4, 9];
  var second := [3, 4, 5];
  Writeln(first);
  Writeln(first + second, ' ', first * second, ' ', first - second);
  var empty: set of integer := [];
  Writeln(empty = [], ' ', first > [], ' ', [] < first);
  Writeln(['a'..'d', 'z']);
  Writeln([False..True]);
  Writeln(Range(1, 4).ToSet, ' ', Arr(2, 1, 2).ToHashSet);
  first += [];
  first *= [2, 4, 9];
  Writeln(first);
  Writeln(SetOf&<integer>());
end.
