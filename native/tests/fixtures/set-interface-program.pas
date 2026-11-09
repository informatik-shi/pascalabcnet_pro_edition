type Remainder=class(IEqualityComparer<integer>)
public
 function Equals(a,b:integer):boolean:=a mod 3=b mod 3;
 function GetHashCode(n:integer):integer:=n mod 3;
end;
TraceSet=class(ISet<integer>)
private
 data:HashSet<integer>;
 function Size:integer;begin Write('C');Result:=data.Count;end;
 function ReadOnly:boolean;begin Write('O');Result:=False;end;
public
 constructor(size:integer;comparer:IEqualityComparer<integer>);begin data:=new HashSet<integer>(comparer);if size<>0 then begin data.Add(1);data.Add(2);end;end;
 property Count:integer read Size;
 property IsReadOnly:boolean read ReadOnly;
 function Add(n:integer):boolean;begin Write('A');Result:=data.Add(n);end;
 procedure ICollection<integer>.Add(n:integer);begin Write('P');data.Add(n);end;
 procedure Clear;begin Write('L');data.Clear;end;
 function Contains(n:integer):boolean;begin Write('H');Result:=data.Contains(n);end;
 procedure CopyTo(a:array of integer;i:integer);begin Write('Y');data.CopyTo(a,i);end;
 function Remove(n:integer):boolean;begin Write('R');Result:=data.Remove(n);end;
 function GetEnumerator:IEnumerator<integer>;begin Write('E');Result:=data.AsEnumerable.GetEnumerator;end;
 procedure ExceptWith(x:IEnumerable<integer>);begin Write('X');data.ExceptWith(x);end;
 procedure IntersectWith(x:IEnumerable<integer>);begin Write('I');data.IntersectWith(x);end;
 function IsProperSubsetOf(x:IEnumerable<integer>):boolean;begin Write('j');Result:=data.IsProperSubsetOf(x);end;
 function IsProperSupersetOf(x:IEnumerable<integer>):boolean;begin Write('J');Result:=data.IsProperSupersetOf(x);end;
 function IsSubsetOf(x:IEnumerable<integer>):boolean;begin Write('s');Result:=data.IsSubsetOf(x);end;
 function IsSupersetOf(x:IEnumerable<integer>):boolean;begin Write('S');Result:=data.IsSupersetOf(x);end;
 function Overlaps(x:IEnumerable<integer>):boolean;begin Write('V');Result:=data.Overlaps(x);end;
 function SetEquals(x:IEnumerable<integer>):boolean;begin Write('Q');Result:=data.SetEquals(x);end;
 procedure SymmetricExceptWith(x:IEnumerable<integer>);begin Write('D');data.SymmetricExceptWith(x);end;
 procedure UnionWith(x:IEnumerable<integer>);begin Write('U');data.UnionWith(x);end;
end;
procedure Items(s:ISet<integer>);begin foreach var n in s do Write(n,',');Writeln;end;
begin
 var kind,mode,size,isNil,operation,item,input,offset:integer;Read(kind,mode,size,isNil,operation,item,input,offset);
 var comparer:IEqualityComparer<integer>;if mode<>0 then comparer:=new Remainder;
 var native:=new HashSet<integer>(comparer);if size<>0 then begin native.Add(1);native.Add(2);end;
 var s:ISet<integer>;if kind=0 then s:=native else s:=new TraceSet(size,comparer);if isNil<>0 then s:=nil;
 var source:IEnumerable<integer>;if input=0 then source:=new integer[0] else if input=1 then source:=Arr(2,3) else if input=2 then source:=Arr(1,4,4) else if input=4 then source:=s;
 if operation=0 then Writeln(s.Count)
 else if operation=1 then Writeln(s.IsReadOnly)
 else if operation=2 then begin Writeln(s.Add(item));Items(s);end
 else if operation=3 then begin s.Clear;Items(s);end
 else if operation=4 then Writeln(s.Contains(item))
 else if operation=5 then begin var a:=new integer[4];s.CopyTo(a,offset);Writeln(a);end
 else if operation=6 then begin Writeln(s.Remove(item));Items(s);end
 else if operation=7 then Items(s)
 else if operation=8 then begin s.ExceptWith(source);Items(s);end
 else if operation=9 then begin s.IntersectWith(source);Items(s);end
 else if operation=10 then Writeln(s.IsProperSubsetOf(source))
 else if operation=11 then Writeln(s.IsProperSupersetOf(source))
 else if operation=12 then Writeln(s.IsSubsetOf(source))
 else if operation=13 then Writeln(s.IsSupersetOf(source))
 else if operation=14 then Writeln(s.Overlaps(source))
 else if operation=15 then Writeln(s.SetEquals(source))
 else if operation=16 then begin s.SymmetricExceptWith(source);Items(s);end
 else if operation=17 then begin s.UnionWith(source);Items(s);end
 else if operation=18 then begin var c:ICollection<integer>:=s;c.Add(item);Items(s);end
 else if operation=19 then begin var c:ICollection<integer>:=s;Writeln(c.Remove(item));Items(s);end
 else if operation=20 then begin var e:IEnumerable<integer>:=s;Writeln(e.Count);end
 else if operation=21 then begin var e:IEnumerable<integer>:=s;Writeln(e.ToArray);end
 else if operation=22 then Items(new HashSet<integer>(s,comparer));
end.
