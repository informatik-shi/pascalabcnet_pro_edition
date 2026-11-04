// Membership operands have concrete types before execution.
begin
  var numbers := Arr(1, 2, 3);
  Writeln(2 in numbers, ' ', 9 not in numbers);
  var list := Lst(4, 5);
  Writeln(4 in list, ' ', 1 not in list);
  Writeln('bc' in 'abcd', ' ', 'BC' not in 'abcd');
  Writeln('abcd'.Contains('bc'), ' ', ''.Contains(''));
  var matrix := new integer[2, 2]((1, 2), (3, 4));
  Writeln(3 in matrix, ' ', 5 not in matrix);
  var calls := 0;
  var sequence1 := SeqGen(10, i -> begin Inc(calls); Result := i; end);
  Writeln(3 in sequence1, ' ', calls);
end.
