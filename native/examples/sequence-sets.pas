type
  SameLastDigit = class(IEqualityComparer<integer>)
  public
    function Equals(a,b:integer):boolean := a mod 10 = b mod 10;
    function GetHashCode(a:integer):integer := a mod 10;
  end;
begin
  var a := Arr(1,2,1,3,2);
  var b := Arr(3,4);
  Writeln(a.Distinct.ToArray);
  Writeln(a.Union(b).ToArray);
  Writeln(a.Intersect(b).ToArray);
  Writeln(a.Except(b).ToArray);
  var policy:IEqualityComparer<integer> := new SameLastDigit;
  Writeln(Arr(1,11,2,12,3).Distinct(policy).ToArray);
  Writeln(a.Union(b).Take(2).ToArray);
end.
