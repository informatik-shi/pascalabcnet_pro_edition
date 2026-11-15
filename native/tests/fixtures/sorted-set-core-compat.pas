uses System;
type Ordered = System.Collections.Generic.SortedSet<integer>;
begin
  var values:=new Ordered(Arr(4,1,3,1,2));
  Writeln(values.Count,' ',values.Min,' ',values.Max);
  foreach var x in values do Write(x,' ');
  Writeln('|');
  Writeln(values.Add(3),' ',values.Add(5),' ',values.Remove(4),' ',values.Remove(99),' ',values.Contains(2));
  var reverse:=System.Collections.Generic.Comparer&<integer>.Create((a,b) -> b-a);
  var down:=new SortedSet<integer>(Arr(4,2,4,3),reverse);
  Writeln(down.Min,' ',down.Max,' ',down.Comparer.Compare(4,2));
  foreach var x in down do Write(x,' ');
  Writeln('|');
  var amounts:=new SortedSet<Decimal>(Arr(Decimal.Parse('4.00'),Decimal.Parse('1.0'),Decimal.Parse('4.0')));
  Writeln(amounts.Count,' ',amounts.Min,' ',amounts.Max);
  Writeln(2 in values,' ',4 in values,' ',values.ToArray.SequenceEqual(Arr(1,2,3,5)));
  var copy:=new Ordered(values);
  var reversedCopy:=new Ordered(down);
  values.Clear;
  Writeln(copy.Count,' ',reversedCopy.Min,' ',reversedCopy.Max);
  Writeln(values.Count,' ',values.Min,' ',values.Max);
end.
