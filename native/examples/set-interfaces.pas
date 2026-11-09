begin
 var numbers:ISet<integer>:=new HashSet<integer>(Arr(1,2));
 Writeln(numbers.Add(2),' ',numbers.Add(3));
 var collection:ICollection<integer>:=numbers;
 collection.Add(4);
 Writeln(numbers.Count,' ',numbers.Contains(4));
 numbers.IntersectWith(Arr(2,4,5));
 Writeln(numbers.SetEquals(Arr(4,2)),' ',numbers.IsProperSubsetOf(Arr(2,4,6)));
 numbers.SymmetricExceptWith(Arr(4,6));
 foreach var n in numbers do Write(n,' ');
 Writeln;
 var source:IEnumerable<integer>:=numbers;
 var same:=ISet&<integer>(source);
 Writeln(same=numbers);
end.
