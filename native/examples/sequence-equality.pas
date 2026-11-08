type
  SameLastDigit = class(IEqualityComparer<integer>)
  public
    function Equals(a,b:integer):boolean := a mod 10 = b mod 10;
    function GetHashCode(a:integer):integer := a mod 10;
  end;
begin
  var a := Arr(11,22,33);
  var b := Arr(1,2,3);
  Writeln(a.SequenceEqual(b));
  var policy:IEqualityComparer<integer> := new SameLastDigit;
  Writeln(SequenceEqual(a,b,policy));
  Writeln(a.SequenceEqual(b,nil));
  Writeln('abc'.SequenceEqual(Arr('a','b','c'),nil));
end.
