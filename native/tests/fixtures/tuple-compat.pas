uses School;
type Pair = (int64,string);
function Data: Pair;
begin
  Result := (42,'answer');
end;
begin
  var p := Data;
  Writeln(p,' ',p[0],' ',p.Item2);
  Writeln((1,2)=(1,2),' ',(1,2)<>(1,3));
  Writeln(Rec(1,'ab',True));
  Writeln(НОДНОК(-18,30));
  Writeln(School.НОДНОК(-9223372036854775808,0));
  var empty: Pair;
  Writeln(empty=nil);
end.
