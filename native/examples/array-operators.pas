// PABCSystem array operators: new arrays, copied records, shared references.
type IntArray = array of integer;

Point = record
  x: integer;
end;

begin
  var a := Arr(1, 2, 3);
  var b := Arr(4, 5);
  Writeln(a + b);
  Writeln(a * 2);
  Writeln(2 * b);
  Writeln(a * 0);
  var p: Point;
  p.x := 7;
  var original := Arr(p);
  var repeated := original * 2;
  repeated[0].x := 9;
  Writeln(original[0].x, ' ', repeated[0].x, ' ', repeated[1].x);
  var inner := Arr(10);
  var nested := new IntArray[1];
  nested[0] := inner;
  var copied := nested + nested;
  copied[0][0] := 20;
  Writeln(inner, ' ', copied[1]);
end.
