type Item=record(IComparable<Item>)
 public X:integer;
 procedure Bump(n:integer);begin X+=n;end;
 function CompareTo(other:Item):integer:=X-other.X;
end;
V=Item;
function Make(n:integer):V;begin Result.X:=n;end;
type CaptureState=class
 public Saved:array of V;Expected,ThrowIndex:integer;
 function ReadCount:integer;begin Write('C');Result:=Expected;end;
 procedure Copy(a:array of V;index:integer);
 begin Write('P');Saved:=a;for var n:=0 to 1 do begin if n=ThrowIndex then begin var z:=0;var failed:=1 div z;end;a[index+n]:=Make(10+n);Write('W');end;end;
 function Enumerate:IEnumerator<V>;begin Write('E');Result:=new List<V>.AsEnumerable.GetEnumerator;end;
end;
Capture=class(ICollection<V>)
 private state:CaptureState;
 public constructor(s:CaptureState);begin state:=s;end;
 property Count:integer read state.ReadCount;
 property IsReadOnly:boolean read boolean(True);
 procedure CopyTo(a:array of V;index:integer);begin state.Copy(a,index);end;
 function GetEnumerator:IEnumerator<V>:=state.Enumerate;
 procedure Add(n:V);begin end;
 procedure Clear;begin end;
 function Contains(n:V):boolean:=False;
 function Remove(n:V):boolean:=False;
end;
procedure PrintValues(a:IEnumerable<V>);begin Writeln(a.Select(v->v.X).ToArray);end;
function Rhs(data:List<V>):integer;begin data[0]:=Make(99);Result:=1;end;
begin
 var count,fail,operation,ctor:integer;Read(count,fail,operation,ctor);
 var state:=new CaptureState;state.Expected:=count;state.ThrowIndex:=fail;
 var source:IEnumerable<V>:=new Capture(state);var data:List<V>;
 if ctor=0 then data:=source.ToList else data:=new List<V>(source);
 Writeln(' ',data.Count,' ',data.Capacity);
 if state.Saved=nil then begin Writeln('empty');Exit;end;
 var it:=data.GetEnumerator;it.MoveNext;
 if operation=1 then state.Saved[0]:=Make(80)
 else if operation=2 then data[0]:=Make(81)
 else if operation=3 then begin data.Add(Make(82));state.Saved[0]:=Make(83);end
 else if operation=4 then begin data.Clear;state.Saved[0]:=Make(84);data.Add(Make(85));end
 else if operation=5 then begin data.Capacity:=count+3;state.Saved[0]:=Make(86);end
 else if operation=6 then System.Array.Reverse(state.Saved)
 else if operation=7 then System.Array.Sort(state.Saved)
 else if operation=8 then state.Saved[0].X+=5
 else if operation=9 then state.Saved[0].Bump(5)
 else if operation=10 then data.Reverse
 else if operation=11 then data.Sort
 else if operation=12 then state.Saved[0].X+=Rhs(data);
 PrintValues(data);PrintValues(state.Saved);Writeln(it.MoveNext);
end.
