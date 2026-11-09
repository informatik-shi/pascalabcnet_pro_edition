type Cursor=record(IEnumerator<integer>)
 private data:array of integer;index:integer;
 public constructor(values:array of integer);begin data:=values;index:=-1;end;
 function MoveNext:boolean;begin Inc(index);Result:=index<data.Length;end;
 property Current:integer read data[index];
 procedure Reset;begin Self:=new Cursor(data);end;
 procedure Dispose;begin Write('D');end;
end;
Bag=record(ICollection<integer>)
 private data:List<integer>;countReads,copyCalls:integer;
 public constructor(seed:integer);begin data:=new List<integer>(Arr(seed,seed+1));countReads:=0;copyCalls:=0;end;
 property Reads:integer read countReads;
 property Copies:integer read copyCalls;
 function Values:array of integer:=data.ToArray;
 function CountValue:integer;begin Inc(countReads);Result:=data.Count;end;
 property Count:integer read CountValue;
 property IsReadOnly:boolean read boolean(False);
 procedure Add(n:integer);begin data.Add(n);end;
 procedure Clear;begin data.Clear;end;
 function Contains(n:integer):boolean:=data.Contains(n);
 function Remove(n:integer):boolean:=data.Remove(n);
 procedure CopyTo(target:array of integer;index:integer);begin Inc(copyCalls);data.CopyTo(target,index);end;
 function GetEnumerator:IEnumerator<integer>:=new Cursor(data.ToArray);
end;
procedure Stats(c:ICollection<integer>;original:Bag);begin var copy:=Bag(c);Writeln(copy.Reads,' ',copy.Copies,' ',original.Reads);end;
begin
 var seed,item,isNil,operation:integer;Read(seed,item,isNil,operation);
 var b:=new Bag(seed);var c:ICollection<integer>:=b;if isNil<>0 then c:=nil;var e:IEnumerable<integer>:=c;
 if operation=0 then begin foreach var n in e do Write(n,';');Writeln;Stats(c,b);end
 else if operation=1 then begin Writeln(e.Count);Stats(c,b);end
 else if operation=2 then begin Writeln(e.ToArray);Stats(c,b);end
 else if operation=3 then begin Writeln(e.ToList.ToArray);Stats(c,b);end
 else if operation=4 then begin var target:=new integer[4];c.CopyTo(target,1);Writeln(target);Stats(c,b);end
 else if operation=5 then begin c.Add(item);Writeln(b.Values);Stats(c,b);end
 else if operation=6 then begin var it:=e.GetEnumerator;var alias:=it;it.MoveNext;Writeln(alias.Current);var copy:=Cursor(it);copy.MoveNext;Writeln(copy.Current,' ',alias.Current);it.Reset;it.MoveNext;Writeln(it.Current);it.Dispose;Writeln;Stats(c,b);end
 else if operation=7 then begin foreach var n in e do begin Write(n,';');break;end;Writeln;Stats(c,b);end
 else if operation=8 then begin var invalid:=e.GetEnumerator;Writeln(invalid.Current);end
 else if operation=9 then begin c.Clear;Writeln(b.Values);Stats(c,b);end
 else if operation=10 then begin var direct:IEnumerable<integer>:=b;var comp:=EqualityComparer<IEnumerable<integer>>.Default;var equal:=comp.Equals(e,direct);if equal then Writeln(c=e,' ',e=direct,' ',equal,' ',comp.GetHashCode(e)=comp.GetHashCode(direct))else Writeln(c=e,' ',e=direct,' ',equal,' -');end
 else if operation=11 then begin var unique:=new HashSet<integer>(e);Writeln(unique.ToArray);Stats(c,b);end
 else if operation=12 then begin Writeln(c.Remove(item));Writeln(b.Values);Stats(c,b);end
 else if operation=13 then begin Writeln(c.Contains(item));Stats(c,b);end;
end.
