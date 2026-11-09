type TraceCollection=class(ICollection<integer>)
private
 data:List<integer>:=new List<integer>;
 function Size:integer;begin Write('C');Result:=data.Count;end;
 function ReadOnly:boolean:=False;
public
 constructor(items:array of integer);begin data.AddRange(items);end;
 property Count:integer read Size;
 property IsReadOnly:boolean read ReadOnly;
 procedure Add(item:integer);begin Write('A');data.Add(item);end;
 procedure Clear;begin Write('L');data.Clear;end;
 function Contains(item:integer):boolean;begin Write('H');Result:=(item=99) or data.Contains(item);end;
 procedure CopyTo(target:array of integer;index:integer);begin Write('P');data.CopyTo(target,index);end;
 function Remove(item:integer):boolean;begin Write('R');Result:=data.Remove(item);end;
 function GetEnumerator:IEnumerator<integer>;begin Write('G');Result:=data.AsEnumerable.GetEnumerator;end;
end;
begin
 var kind,operation,size,isNil,index,length:integer;
 Read(kind,operation,size,isNil,index,length);
 var items:=new integer[size];for var i:=0 to size-1 do items[i]:=i+1;
 var dictionary:=new Dictionary<integer,integer>;
 foreach var item in items do dictionary.Add(item,item);
 var c:ICollection<integer>;
 if kind=0 then c:=items
 else if kind=1 then c:=new List<integer>(items)
 else if kind=2 then c:=new HashSet<integer>(items)
 else if kind=3 then c:=dictionary.Keys
 else if kind=4 then c:=dictionary.Values
 else c:=new TraceCollection(items);
 if isNil<>0 then c:=nil;
 var target:array of integer;if length>=0 then target:=new integer[length];
 var e:IEnumerable<integer>:=c;
 if operation=0 then Writeln(c.Count)
 else if operation=1 then Writeln(c.IsReadOnly)
 else if operation=2 then begin c.Add(9);Writeln(c.Count);end
 else if operation=3 then begin c.Clear;Writeln(c.Count);end
 else if operation=4 then Writeln(c.Contains(2))
 else if operation=5 then begin c.CopyTo(target,index);Writeln(target);end
 else if operation=6 then Writeln(c.Remove(2),' ',c.Count)
 else if operation=7 then begin var iterator:=c.GetEnumerator;while iterator.MoveNext do Write(iterator.Current,',');iterator.Dispose;Writeln;end
 else if operation=8 then Writeln(e.Count)
 else if operation=9 then Writeln(e.Contains(99))
 else if operation=10 then Writeln(e.Contains(99,nil))
 else if operation=11 then Writeln(e.ToArray)
 else if operation=12 then Writeln(e.Any);
end.
