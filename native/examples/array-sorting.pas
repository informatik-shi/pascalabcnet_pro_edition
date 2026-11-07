begin
  var values:=Arr(9,3,1,2,8);
  System.Array.Sort(values,1,3);
  Writeln(values);
  Array.Sort(values,Comparer<integer>.Create((a,b)->b.CompareTo(a)));
  Writeln(values);
  System.Array.Sort&<integer>(values);
  Writeln(values);
end.
