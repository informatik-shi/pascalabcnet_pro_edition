type TraceList=class(IList<integer>)
private
 data:List<integer>:=new List<integer>;
 function Size:integer;begin Write('C');Result:=data.Count;end;
 function ReadOnly:boolean:=False;
 function GetItem(i:integer):integer;begin Write('G',i,';');Result:=data[i];end;
 procedure SetItem(i,value:integer);begin Write('S',i,';');data[i]:=value;end;
public
 constructor(items:array of integer);begin data.AddRange(items);end;
 property Count:integer read Size;
 property IsReadOnly:boolean read ReadOnly;
 property Item[i:integer]:integer read GetItem write SetItem; default;
 procedure Add(x:integer);begin Write('A');data.Add(x);end;
 procedure Clear;begin Write('L');data.Clear;end;
 function Contains(x:integer):boolean;begin Write('H');Result:=data.Contains(x);end;
 procedure CopyTo(a:array of integer;i:integer);begin Write('P');data.CopyTo(a,i);end;
 function Remove(x:integer):boolean;begin Write('R');Result:=data.Remove(x);end;
 function IndexOf(x:integer):integer;begin Write('X');Result:=data.IndexOf(x);end;
 procedure Insert(i,x:integer);begin Write('I',i,';');data.Insert(i,x);end;
 procedure RemoveAt(i:integer);begin Write('D',i,';');data.RemoveAt(i);end;
 function GetEnumerator:IEnumerator<integer>;begin Write('E');Result:=data.AsEnumerable.GetEnumerator;end;
end;
procedure ShowItems(c:IList<integer>);begin foreach var x in c do Write(x,',');Writeln;end;
function Index(i:integer):integer;begin Write('I');Result:=i;end;
function Value:integer;begin Write('V');Result:=9;end;
function Receiver(c:IList<integer>):IList<integer>;begin Write('O');Result:=c;end;
begin
 var kind,operation,size,isNil,indexValue:integer;Read(kind,operation,size,isNil,indexValue);
 var items:=new integer[size];for var i:=0 to size-1 do items[i]:=i+1;
 var c:IList<integer>;
 if kind=0 then c:=items else if kind=1 then c:=new List<integer>(items) else c:=new TraceList(items);
 if isNil<>0 then c:=nil;
 if operation=0 then Writeln(c.Count)
 else if operation=1 then Writeln(c.IsReadOnly)
 else if operation=2 then begin c.Add(9);ShowItems(c);end
 else if operation=3 then begin c.Clear;ShowItems(c);end
 else if operation=4 then Writeln(c.Contains(2))
 else if operation=5 then begin var a:=new integer[3];c.CopyTo(a,indexValue);Writeln(a);end
 else if operation=6 then begin Writeln(c.Remove(2));ShowItems(c);end
 else if operation=7 then ShowItems(c)
 else if operation=8 then Writeln(c[indexValue])
 else if operation=9 then begin c[indexValue]:=9;ShowItems(c);end
 else if operation=10 then Writeln(c.IndexOf(2))
 else if operation=11 then begin c.Insert(indexValue,9);ShowItems(c);end
 else if operation=12 then begin c.RemoveAt(indexValue);ShowItems(c);end
 else if operation=13 then begin Receiver(c)[Index(indexValue)]+=Value();ShowItems(c);end;
end.
