program TupleLiteralDeclarationsSource;
var trace := 0;
    (globalLeft,globalRight) := (10,globalLeft+4,discardedName);
type Sample = class
public
  Value := globalRight;
end;
function Mark(n: integer): integer;
begin
  trace:=trace*10+n;
  Result:=n;
end;
begin
  Writeln(globalLeft,' ',globalRight);
  var holder := new Sample;
  Writeln(holder.Value);
  var (leftPart,rightPart) := (Mark(1),leftPart+Mark(2),Mark(3));
  Writeln(leftPart,' ',rightPart,' ',trace);
end.
