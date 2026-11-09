type R=CharRange;T=char;C=ICollection<T>;E=IEnumerable<T>;Eq=IEquatable<R>;
procedure Items(data:E);begin foreach var n in data do Write(Ord(n),',');Writeln;end;
begin
 var low,high,operation,item,offset,length,isNil,nilArray:integer;Read(low,high,operation,item,offset,length,isNil,nilArray);
 var r:=new R(Chr(low and 65535),Chr(high and 65535));var original:=r;
 var a:C:=r;var b:C:=a;var fresh:C:=r;if isNil<>0 then a:=nil;
 var value:T:=Chr(item and 65535);
 if operation=0 then Writeln(a.Count)
 else if operation=1 then Writeln(a.IsReadOnly)
 else if operation=2 then Writeln(a.Contains(value))
 else if operation=3 then begin var data:=new T[length];if nilArray<>0 then data:=nil;a.CopyTo(data,offset);Writeln(data.Select(n->Ord(n)).ToArray);end
 else if operation=4 then Items(a)
 else if operation=5 then a.Add(value)
 else if operation=6 then Writeln(a.Remove(value))
 else if operation=7 then a.Clear
 else if operation=8 then begin Writeln(a=b,' ',a=fresh);r:=new R(Chr((low+1) and 65535),Chr((high+1) and 65535));Writeln(R(a)=original);end
 else if operation=9 then begin var copy:=R(a);Writeln(Ord(copy.Low),' ',Ord(copy.High),' ',copy=r);end
 else if operation=10 then begin var e:E:=a;var again:E:=a;var direct:E:=r;Writeln(a=e,' ',e=again,' ',e=direct);end
 else if operation=11 then begin var e:=Eq(a);Writeln(e.Equals(r));end
 else if operation=12 then begin var comparer:=EqualityComparer<C>.Default;Writeln(comparer.Equals(a,fresh),' ',comparer.GetHashCode(a)=comparer.GetHashCode(fresh));end
 else if operation=13 then begin var e:E:=a;Writeln(e.Count);var data:=e.ToArray;Writeln(data.Select(n->Ord(n)).ToArray);end
 else if operation=14 then begin var other:C:=Arr(Chr(low and 65535),Chr(high and 65535));var list:=new List<C>(Arr(a));var unique:=new HashSet<C>(Arr(a,fresh));Writeln(list.Contains(other),' ',unique.Count);end;
end.
