type RangeEquality = IEquatable<RealRange>;
begin
  var r := new RealRange(-1.5,2.5);
  var boxed: RangeEquality := r;
  var same := boxed;
  var fresh: RangeEquality := r;
  Writeln(boxed=same,' ',boxed=fresh);
  Writeln(boxed.Equals(r));
  var restored := RealRange(boxed);
  Writeln(restored.Low,' ',restored.High);
  var comparer := EqualityComparer<RangeEquality>.Default;
  Writeln(comparer.Equals(boxed,fresh),' ',comparer.GetHashCode(boxed)=comparer.GetHashCode(fresh));
end.
