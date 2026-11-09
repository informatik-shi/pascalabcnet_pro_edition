type ICounter=interface
 property Value:integer read write;
 function Add(n:integer):integer;
 procedure Replace(n:integer);
end;
ITrace=interface(ICounter) property Calls:integer read;function SameValue(other:Counter):boolean;function Rebox:ICounter;end;
Counter=record(ITrace,IEquatable<Counter>)
 private x,ticks:integer;
 public constructor(n:integer);begin x:=n;ticks:=0;end;
 property Value:integer read x write begin x:=value;Inc(ticks);end;
 property Calls:integer read ticks;
 property ICounter.Value:integer read x+100 write begin x:=value-100;Inc(ticks);end;
 property Item[i:integer]:integer read x+i write begin x:=value-i;Inc(ticks);end;default;
 function Add(n:integer):integer;begin x+=n;Inc(ticks);Result:=x;end;
 function ICounter.Add(n:integer):integer;begin x+=n*2;Inc(ticks);Result:=x+100;end;
 procedure Replace(n:integer);begin Self:=new Counter(n);end;
 procedure ICounter.Replace(n:integer);begin Self:=new Counter(n+1);end;
 procedure Change(var n:integer);begin n+=x;Inc(x);Inc(ticks);end;
 function Equals(other:Counter):boolean;begin Inc(ticks);Result:=x=other.x;end;
 function SameValue(other:Counter):boolean:=Self=other;
 function Rebox:ICounter:=Self;
end;
Holder=class public Data:Counter;constructor(n:integer);begin Data:=new Counter(n);end;end;
Fake=class(ITrace)
 private x,ticks:integer;
 public constructor(n:integer);begin x:=n;end;
 property Value:integer read x+200 write begin x:=value-200;Inc(ticks);end;
 property Calls:integer read ticks;
 function Add(n:integer):integer;begin x+=n*3;Inc(ticks);Result:=x+200;end;
 procedure Replace(n:integer);begin x:=n+2;ticks:=0;end;
 function SameValue(other:Counter):boolean:=False;
 function Rebox:ICounter:=Self;
end;
Eq=IEquatable<Counter>;
procedure State(c:Counter);begin Writeln(c.Value,' ',c.Calls);end;
procedure CopyChange(c:Counter;n:integer);begin c.Add(n);State(c);end;
procedure RefChange(var c:Counter;n:integer);begin c.Add(n);end;
function Copy(c:Counter):Counter:=c;
begin
 var seed,n,route,operation:integer;Read(seed,n,route,operation);
 var c:=new Counter(seed);var copied:=c;var a:ICounter:=c;var b:=a;var fresh:ICounter:=c;
 if route=1 then a:=nil;if route=2 then a:=fresh;if route=3 then a:=new Counter(seed+1);if route=4 then a:=new Fake(seed);
 Writeln(a=b,' ',a=fresh);
 if operation=0 then begin c.Add(n);State(c);State(copied);end
 else if operation=1 then begin CopyChange(c,n);State(c);end
 else if operation=2 then begin RefChange(c,n);State(c);end
 else if operation=3 then begin Writeln(Copy(c).Add(n));State(c);end
 else if operation=4 then begin var data:=Arr(c);data[0].Add(n);State(data[0]);State(c);end
 else if operation=5 then begin var holder:=new Holder(seed);holder.Data.Add(n);State(holder.Data);end
 else if operation=6 then begin c.Value+=n;State(c);end
 else if operation=7 then begin c[n]+=n;State(c);end
 else if operation=8 then begin a.Value+=n;Writeln(a.Value,' ',b.Value,' ',fresh.Value);end
 else if operation=9 then begin Writeln(a.Add(n));Writeln(a.Value,' ',b.Value,' ',fresh.Value);State(c);end
 else if operation=10 then begin a.Replace(n);Writeln(a.Value,' ',b.Value,' ',ITrace(a).Calls);end
 else if operation=11 then begin var unboxed:=Counter(a);unboxed.Add(n);State(unboxed);Writeln(a.Value);State(c);end
 else if operation=12 then begin var trace:=ITrace(a);Writeln(trace.Value,' ',trace.Calls,' ',a=trace);end
 else if operation=13 then begin var eq:=Eq(a);Writeln(eq.Equals(c));Writeln(EqualityComparer<Counter>.Default.Equals(c,new Counter(seed+n)),' ',c.Calls);Writeln(ITrace(a).Calls);end
 else if operation=14 then begin var data:=new List<Counter>(Arr(c));Writeln(data.Contains(new Counter(seed+n)));State(data[0]);State(c);end
 else if operation=15 then begin var comp:=EqualityComparer<ICounter>.Default;var equal:=comp.Equals(a,fresh);if equal then Writeln(equal,' ',comp.GetHashCode(a)=comp.GetHashCode(fresh))else Writeln(equal,' -');end
 else if operation=16 then begin var slots:=Arr(a,b,fresh);Writeln(slots[0].Add(n));Writeln(slots[0].Value,' ',slots[1].Value,' ',slots[2].Value);end
 else if operation=17 then begin c.Replace(n);State(c);State(copied);end
 else if operation=18 then begin var value:=n;c.Change(value);Writeln(value);State(c);end
 else if operation=19 then begin var records:=new List<Counter>(Arr(c));records[0].Add(n);State(records[0]);State(c);end
 else if operation=20 then Writeln(ITrace(a).SameValue(c))
 else if operation=21 then begin var again:=ITrace(a).Rebox;Writeln(a=again);again.Add(n);Writeln(a.Value,' ',again.Value);end;
end.
