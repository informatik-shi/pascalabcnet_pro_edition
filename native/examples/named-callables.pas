type
  Check = function(n: integer): boolean;
  OtherCheck = function(n: integer): boolean;
  Arrow = integer -> boolean;
  RefFirst = procedure(var n: integer);
  RefSecond = procedure(var n: integer);
function Positive(n: integer): boolean := n > 0;
procedure Bump(var n: integer);
begin
  Inc(n);
end;
begin
  Writeln(typeof(Check).Name, ' ', typeof(Check) = typeof(Arrow));
  Writeln(typeof(Check).FullName);
  Writeln(typeof(Arrow) = typeof(Func<integer,boolean>));
  var p: Check := Positive;
  var a: OtherCheck := p;
  var b: OtherCheck := p;
  Writeln(a(2), ' ', a = b);
  var back: Check := a;
  Writeln(back = p);
  var first: RefFirst := Bump;
  var second: RefSecond := first;
  var n := 4;
  second(n);
  Writeln(n, ' ', second.GetType.Name);
end.
