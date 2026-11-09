type P=Predicate<integer>;F=Func<integer,boolean>;
function Positive(n:integer):boolean:=n>=0;
begin
 Writeln(typeof(P)=typeof(F));
 var p:P:=Positive;
 var q:P:=Positive;
 var first:F:=p;
 var same:F:=p;
 var other:F:=q;
 Writeln(p.GetType=typeof(P),' ',first.GetType=typeof(F));
 Writeln(EqualityComparer<F>.Default.Equals(first,same),' ',EqualityComparer<F>.Default.Equals(first,other));
 Writeln(new HashSet<F>(Arr(first,same,other)).Count);
 p:=nil;
 Writeln(first(3));
end.
