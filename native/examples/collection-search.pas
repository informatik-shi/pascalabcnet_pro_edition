type
  SameLastDigit = class(IEqualityComparer<integer>)
  public
    function Equals(a,b:integer):boolean := a mod 10 = b mod 10;
    function GetHashCode(a:integer):integer := a mod 10;
  end;
begin
  var a := Arr(2,12,3,2);
  Writeln(Array.IndexOf(a,2), ' ', System.Array.LastIndexOf(a,2));
  Writeln(Array.IndexOf(a,2,1,3), ' ', Array.LastIndexOf(a,2,2,3));
  var l := Lst(2,12,3,2);
  Writeln(l.IndexOf(2), ' ', l.LastIndexOf(2), ' ', l.Remove(12));
  var policy:IEqualityComparer<integer> := new SameLastDigit;
  var s := new HashSet<integer>(Arr(2),policy);
  Writeln(s.AsEnumerable.Contains(12), ' ', s.AsEnumerable.Contains(12,nil));
  Writeln(a.Contains(22,policy), ' ', Contains(l,22,policy));
end.
