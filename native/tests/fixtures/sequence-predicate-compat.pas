function Even(value:integer):boolean;
begin Result:=value mod 2=0; end;
function Square(value:integer):integer;
begin Result:=value*value; end;
begin
  var s:=Range(1,5);
  Writeln(s.Any(Even),' ',s.All(Even),' ',s.Count(Even));
  Writeln(s.First(Even),' ',s.Last(Even),' ',Seq(1,2,3).Single(Even));
  Writeln(s.Sum(Square),' ',s.Average(Square));
  Writeln((new integer[0]).All(Even),' ',(new integer[0]).FirstOrDefault(Even));
end.
