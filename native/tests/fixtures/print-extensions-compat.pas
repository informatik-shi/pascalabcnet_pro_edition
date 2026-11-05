// Print methods return their source; sequence output has no set braces.
begin
  var values := [1, 2, 3];
  Exclude(values, 2);
  Include(values, 4);
  Writeln(values);
  values.Println;
  var original := values.PrintLines(x -> x * 10);
  Writeln(original.Sum);
  PrintDelimDefault := ' | ';
  Arr(4, 5, 6).Println;
  var matrix := new integer[2, 2]((1, 20), (300, 4));
  matrix.Print;
  PrintMatrixWithFormat := False;
  matrix.Print;
end.
