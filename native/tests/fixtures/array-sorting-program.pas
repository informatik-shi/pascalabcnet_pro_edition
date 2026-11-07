type
 Item=class(IComparable<Item>)
 public
  id,score:integer;
  constructor Create(i,value:integer);begin id:=i;score:=value;end;
  function CompareTo(other:Item):integer;
  begin Touch('C('+Tag(Self)+','+Tag(other)+');');if mode=4 then Result:=-1 else if other=nil then Result:=1 else if mode=1 then Result:=other.score-score else Result:=score-other.score;end;
 end;
var items:array of Item;mode,calls:integer;
function Tag(x:Item):string;begin if x=nil then Result:='nil' else Result:=IntToStr(x.id)+':'+IntToStr(x.score);end;
function Dump:string;
begin Result:='[';for var i:=0 to items.Length-1 do begin if i<>0 then Result+=',';Result+=Tag(items[i]);end;Result+=']';end;
procedure Touch(text:string);
begin
 Write(text);calls+=1;if (mode=2) or (mode=3) then Write(Dump,';');
 if calls<>3 then Exit;
 if mode=2 then begin var zero:=0;var q:=1 div zero;end;
 if mode=3 then begin var q:=Arr(1)[9];end;
 if (mode=5) and (items.Length>1) then begin var x:=items[0];items[0]:=items[items.Length-1];items[items.Length-1]:=x;Write('S;');end;
 if mode=6 then begin foreach var x in items do if x<>nil then begin x.score+=7;break;end;Write('V;');end;
 if (mode=7) and (items.Length>0) then begin items[0]:=new Item(999,-77);Write('R;');end;
 if mode=8 then begin items:=Arr(new Item(777,42));Write('A;');end;
end;
function Compare(a,b:Item;labelText:string):integer;
begin Touch(labelText+'('+Tag(a)+','+Tag(b)+');');if mode=4 then Result:=-1 else if a=nil then begin if b=nil then Result:=0 else Result:=-1;end else if b=nil then Result:=1 else if mode=1 then Result:=b.score-a.score else Result:=a.score-b.score;end;
function CompareItems(a,b:Item):integer:=Compare(a,b,'F');
type
 IOrder=interface(IComparer<Item>) end;
 Order=class(IComparer<Item>) public function Compare(a,b:Item):integer:=CompareOrder(a,b);end;
 NamedOrder=class(IOrder) public function Compare(a,b:Item):integer:=CompareNamed(a,b);end;
function CompareOrder(a,b:Item):integer:=Compare(a,b,'I');
function CompareNamed(a,b:Item):integer:=Compare(a,b,'N');
var scenario,size,seed,nils,start,length:integer;
function Killer(i,n:integer):integer;
begin if i<n div 2 then Result:=2*i else Result:=2*(n-i)-1;end;
begin
 Readln(scenario,size,seed,mode,nils,start,length);items:=new Item[size];
 for var i:=0 to size-1 do if (nils=0) or ((i+seed) mod 5<>0) then items[i]:=new Item(i,if scenario>=11 then Killer(i,size) else (i*17+seed*13) mod 7-3);
 if scenario=0 then System.Array.Sort(items);
 if scenario=1 then System.Array.Sort(items,CompareItems);
 if scenario=2 then Array.Sort(items,Comparer<Item>.Default);
 if scenario=3 then System.Array.Sort(items,Comparer<Item>.Create(CompareItems));
 if scenario=4 then System.Array.Sort(items,new Order);
 if scenario=5 then Array.Sort(items,new NamedOrder);
 if (scenario=6) or (scenario=11) then System.Array.Sort(items,start,length);
 if scenario=7 then System.Array.Sort(items,start,length,nil);
 if (scenario=8) or (scenario=12) then System.Array.Sort(items,start,length,new Order);
 if scenario=9 then begin var order:IComparer<Item>;System.Array.Sort(items,order);end;
 if scenario=10 then begin var compare:function(a,b:Item):integer;System.Array.Sort(items,compare);end;
 Write('END',Dump);
end.
