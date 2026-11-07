type
 Item=class(IComparable<Item>)
 public
  id,score:integer;
  constructor Create(i,value:integer);begin id:=i;score:=value;end;
  function CompareTo(other:Item):integer;
  begin Touch('C('+Tag(Self)+','+Tag(other)+');');if mode=4 then Result:=-1 else if other=nil then Result:=1 else if mode=1 then Result:=other.score-score else Result:=score-other.score;end;
 end;
 IOrder=interface(IComparer<Item>) end;
 Order=class(IComparer<Item>) public function Compare(a,b:Item):integer:=CompareOrder(a,b);end;
 NamedOrder=class(IOrder) public function Compare(a,b:Item):integer:=CompareNamed(a,b);end;
 GuardCursor=class(IEnumerator<integer>)
 private n:integer;
 public function MoveNext:boolean;begin n+=1;Result:=n=1;end;
  property Current:integer read Zero;function Zero:integer:=0;
  procedure Reset;begin n:=0;end;
  procedure Dispose;begin Finish;end;
 end;
 GuardSource=class(IEnumerable<integer>) public function GetEnumerator:IEnumerator<integer>:=new GuardCursor;end;
var keys,items,oldKeys,oldItems:array of Item;mode,calls,scenario,size,kind,nils,start,length:integer;
function Tag(x:Item):string;begin if x=nil then Result:='nil' else Result:=IntToStr(x.id)+':'+IntToStr(x.score);end;
function Dump(a:array of Item):string;
begin if a=nil then begin Result:='nil';Exit;end;Result:='[';for var i:=0 to a.Length-1 do begin if i<>0 then Result+=',';Result+=Tag(a[i]);end;Result+=']';end;
procedure Touch(text:string);
begin
 Write(text);calls+=1;if (mode=2) or (mode=3) then Write(Dump(keys),' ',Dump(items),';');
 if calls<>3 then Exit;
 if mode=2 then begin var zero:=0;var q:=1 div zero;end;
 if mode=3 then begin var q:=Arr(1)[9];end;
 if (mode=5) and (keys.Length>1) then begin var x:=keys[0];keys[0]:=keys[keys.Length-1];keys[keys.Length-1]:=x;Write('S;');end;
 if mode=6 then begin foreach var x in keys do if x<>nil then begin x.score+=7;break;end;Write('V;');end;
 if (mode=7) and (keys.Length>0) then begin keys[0]:=new Item(999,-77);Write('R;');end;
 if mode=8 then begin keys:=Arr(new Item(777,42));Write('A;');end;
 if (mode=9) and (items<>nil) and (items.Length>1) then begin var x:=items[0];items[0]:=items[items.Length-1];items[items.Length-1]:=x;Write('T;');end;
 if (mode=10) and (items<>nil) and (items.Length>0) then begin items[0]:=new Item(888,-66);Write('B;');end;
 if mode=11 then begin items:=Arr(new Item(666,24));Write('D;');end;
 if mode=12 then begin items:=nil;Write('Z;');end;
 if (mode=13) and (items<>nil) then begin foreach var x in items do if x<>nil then begin x.score+=11;break;end;Write('W;');end;
 if mode=14 then begin keys:=nil;Write('H;');end;
end;
function Compare(a,b:Item;labelText:string):integer;
begin Touch(labelText+'('+Tag(a)+','+Tag(b)+');');if mode=4 then Result:=-1 else if a=nil then begin if b=nil then Result:=0 else Result:=-1;end else if b=nil then Result:=1 else if mode=1 then Result:=b.score-a.score else Result:=a.score-b.score;end;
function CompareItems(a,b:Item):integer:=Compare(a,b,'F');
function CompareOrder(a,b:Item):integer:=Compare(a,b,'I');
function CompareNamed(a,b:Item):integer:=Compare(a,b,'N');
procedure Finish;
begin Write('|',Dump(keys),' ',Dump(items),' ',Dump(oldKeys),' ',Dump(oldItems),' ',keys=oldKeys,' ',items=oldItems);end;
function Killer(i,n:integer):integer;
begin if i<n div 2 then Result:=2*i else Result:=2*(n-i)-1;end;
begin
 Readln(scenario,size,kind,mode,nils,start,length);keys:=new Item[size];
 for var i:=0 to size-1 do if (nils=0) or ((i+17) mod 5<>0) then keys[i]:=new Item(i,if scenario>=8 then Killer(i,size) else (i*17+17*13) mod 7-3);
 var itemCount:=if kind=3 then size div 2 else if kind=4 then size+3 else if kind=6 then 0 else size;
 items:=new Item[itemCount];for var i:=0 to itemCount-1 do if (nils=0) or (i mod 3<>0) then begin if kind=7 then items[i]:=keys[i] else items[i]:=new Item(100+i,i mod 5);end;
 if kind=1 then items:=keys;if kind=2 then items:=nil;if kind=5 then keys:=nil;oldKeys:=keys;oldItems:=items;
 foreach var ignored in new GuardSource do begin
  if scenario=0 then System.Array.Sort(keys,items);
  if scenario=1 then Array.Sort(keys,items,Comparer<Item>.Default);
  if scenario=2 then System.Array.Sort(keys,items,Comparer<Item>.Create(CompareItems));
  if scenario=3 then Array.Sort(keys,items,new Order);
  if scenario=4 then System.Array.Sort(keys,items,new NamedOrder);
  if (scenario=5) or (scenario=8) then Array.Sort(keys,items,start,length);
  if scenario=6 then System.Array.Sort(keys,items,start,length,nil);
  if (scenario=7) or (scenario=9) then Array.Sort(keys,items,start,length,new Order);
 end;
end.
