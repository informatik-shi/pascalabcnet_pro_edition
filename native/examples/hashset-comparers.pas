type
 RemainderEquality = class(IEqualityComparer<integer>)
 public
  function Equals(a,b:integer):boolean := a mod 10 = b mod 10;
  function GetHashCode(a:integer):integer := a mod 10;
 end;
begin
 var comparison:IEqualityComparer<integer> := new RemainderEquality;
 var numbers := new HashSet<integer>(Arr(1,11,2,12),comparison);
 Writeln(numbers.Count,' ',numbers.Contains(21));
 var actual:integer;
 Writeln(numbers.TryGetValue(31,actual),' ',actual);
 var preserved := Copy(numbers);
 var ordinary := new HashSet<integer>(numbers);
 Writeln(preserved.Comparer=comparison,' ',preserved.Add(41));
 Writeln(ordinary.Comparer=EqualityComparer<integer>.Default,' ',ordinary.Add(41));
end.
