type R=RealRange;E=IEquatable<R>;TPair=(R,integer);
function Item(n:integer):R;
begin
 if n=0 then Result:=new R(0,1)
 else if n=1 then Result:=new R(1,0)
 else if n=2 then Result:=new R(-0.0,0.0)
 else if n=3 then Result:=new R(0.0,-0.0)
 else if n=4 then Result:=new R(-1,1)
 else if n=5 then Result:=new R(1,1)
 else if n=6 then Result:=new R(5e-324,1)
 else if n=7 then Result:=new R(1.7976931348623157e308,1.7976931348623157e308);
end;
begin
 var ai,bi,nilMode:integer;Read(ai,bi,nilMode);var ra:=Item(ai);var rb:=Item(bi);
 var a:E:=ra;var b:E:=rb;if (nilMode and 1)<>0 then a:=nil;if (nilMode and 2)<>0 then b:=nil;var same:=a;
 Writeln(ra=rb);Writeln(EqualityComparer<R>.Default.Equals(ra,rb));
 Writeln(a=same,' ',a=b);
 var comparer:=EqualityComparer<E>.Default;Writeln(comparer.Equals(a,b),' ',comparer.GetHashCode(a)=comparer.GetHashCode(b));
 Writeln(comparer.Equals(a,a));
 Writeln(EqualityComparer<TPair>.Default.Equals((ra,1),(rb,1)));
 var records:=new List<R>(Arr(ra));var recordSet:=new HashSet<R>(Arr(ra,rb));Writeln(records.Contains(rb),' ',recordSet.Count);
 var boxes:=new List<E>(Arr(a));var boxSet:=new HashSet<E>(Arr(a,b));Writeln(boxes.Contains(b),' ',boxSet.Count);
 Writeln(a.Equals(rb));Writeln(R(a).Equals(ra));
end.
