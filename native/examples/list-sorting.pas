begin
  var values:=Lst(5,1,4,2,3);
  values.Sort(Comparer<integer>.Create((a,b)->b.CompareTo(a)));
  Writeln(values);
  var previous:=values;
  Sort(values,x->x);
  Writeln(values);
  Writeln(previous);
  Writeln(values=previous);
  var natural:IComparer<integer>;
  values.Sort(1,3,natural);
  Writeln(values);
end.
