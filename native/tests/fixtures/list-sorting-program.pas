type
 RecordData=record n:integer;end;
 Item=class(IComparable<Item>)
 public id,score:integer;
  constructor Create(i,n:integer);begin id:=i;score:=n;end;
  function CompareTo(other:Item):integer;
  begin Touch('C('+Tag(Self)+','+Tag(other)+');');if mode=4 then Result:=-1 else if other=nil then Result:=1 else if mode=1 then Result:=other.score-score else Result:=score-other.score;end;
 end;
 Key=class(IComparable<Key>)
 public id,score:integer;
  constructor Create(i,n:integer);begin id:=i;score:=n;end;
  function CompareTo(other:Key):integer;
  begin Touch('Q('+KeyTag(Self)+','+KeyTag(other)+');');if mode=4 then Result:=-1 else if other=nil then Result:=1 else if mode=1 then Result:=other.score-score else Result:=score-other.score;end;
 end;
 NamedOrder=interface(IComparer<Item>) end;
 Order=class(NamedOrder) public function Compare(a,b:Item):integer:=CompareItems(a,b,'U');end;
 GuardCursor=class(IEnumerator<integer>)
 private n:integer;
 public function MoveNext:boolean;begin n+=1;Result:=n=1;end;
  property Current:integer read Zero;function Zero:integer:=0;
  procedure Reset;begin n:=0;end;
  procedure Dispose;begin Finish;end;
 end;
 GuardSource=class(IEnumerable<integer>) public function GetEnumerator:IEnumerator<integer>:=new GuardCursor;end;
 EmptyCursor=class(IEnumerator<Item>)
 public function MoveNext:boolean:=False;
  property Current:Item read None;function None:Item:=nil;
  procedure Reset;begin end;
  procedure Dispose;begin var next:=oldCursor.MoveNext;Write('Y',next,';');end;
 end;
 EmptySource=class(IEnumerable<Item>) public function GetEnumerator:IEnumerator<Item>:=new EmptyCursor;end;
var lst,alias:List<Item>;oldCursor:IEnumerator<Item>;mode,calls,scenario,size,pad,nils,range_index,range_count:integer;records:List<RecordData>;
function Tag(x:Item):string;begin if x=nil then Result:='nil' else Result:=IntToStr(x.id)+':'+IntToStr(x.score);end;
function KeyTag(x:Key):string;begin if x=nil then Result:='nil' else Result:=IntToStr(x.id)+':'+IntToStr(x.score);end;
function Dump(data:List<Item>):string;
begin if data=nil then begin Result:='nil';Exit;end;Result:='[';for var i:=0 to data.Count-1 do begin if i>0 then Result+=',';Result+=Tag(data[i]);end;Result+=']';end;
procedure Finish;
begin Write('Z',Dump(lst),'|',Dump(alias),'|',lst=alias,'|E');var next:=oldCursor.MoveNext;Write(next);if next then Write(':',Tag(oldCursor.Current));Write(';');end;
function Plain(a,b:Item):integer;
begin if a=nil then begin if b=nil then Result:=0 else Result:=-1;end else if b=nil then Result:=1 else Result:=a.score.CompareTo(b.score);end;
procedure Touch(text:string);
begin
 Write(text);calls+=1;if (mode=2) or (mode=3) then Write(Dump(lst),';');
 if calls<>3 then Exit;
 if mode=2 then begin var zero:=0;var q:=1 div zero;end;
 if mode=3 then begin var missing:=lst[999999];end;
 if (mode=5) and (lst.Count>0) then begin lst[0]:=new Item(999,-77);Write('R;');end;
 if mode=6 then begin foreach var x in lst do if x<>nil then begin x.score+=7;break;end;Write('V;');end;
 if (mode=7) and (lst.Count>1) then begin var x:=lst[0];lst[0]:=lst[lst.Count-1];lst[lst.Count-1]:=x;Write('S;');end;
 if mode=8 then begin lst.Add(new Item(998,-66));Write('A;');end;
 if mode=9 then begin lst.Capacity:=lst.Count+9;Write('B;');end;
 if mode=10 then begin lst.Clear;Write('C;');end;
 if (mode=11) and (lst.Count>0) then begin lst.RemoveAt(0);Write('D;');end;
 if mode=12 then begin lst.Insert(0,new Item(997,-55));Write('I;');end;
 if mode=13 then begin lst.RemoveRange(0,Min(2,lst.Count));Write('M;');end;
 if mode=14 then begin lst.InsertRange(Min(1,lst.Count),Arr(new Item(996,-44),new Item(995,44)));Write('J;');end;
 if mode=15 then begin lst.AddRange(new Item[0]);Write('N;');end;
 if mode=16 then begin lst.TrimExcess;Write('T;');end;
 if mode=17 then begin lst:=new List<Item>(lst);Write('G;');end;
 if mode=18 then begin lst.Sort(Plain);Write('H;');end;
 if (mode=19) and (lst.Count>0) then begin lst.RemoveAt(0);lst.Add(new Item(994,-33));Write('P;');end;
 if mode=20 then begin lst.Clear;lst.Add(new Item(993,-22));lst.Add(new Item(992,22));Write('W;');end;
 if mode=21 then begin lst.AddRange(new EmptySource);Write('O;');end;
 if mode=22 then begin lst:=nil;Write('X;');end;
end;
function CompareItems(a,b:Item;labelText:string):integer;
begin Touch(labelText+'('+Tag(a)+','+Tag(b)+');');if mode=4 then Result:=-1 else if a=nil then begin if b=nil then Result:=0 else Result:=-1;end else if b=nil then Result:=1 else if mode=1 then Result:=b.score-a.score else Result:=a.score-b.score;end;
function CompareFunction(a,b:Item):integer:=CompareItems(a,b,'F');
function Less(a,b:Item):boolean:=CompareItems(a,b,'L')<0;
function IntKey(x:Item):integer;begin Touch('K('+Tag(x)+');');if x=nil then Result:=-99 else Result:=x.score mod 3;end;
function ClassKey(x:Item):Key;begin Touch('K('+Tag(x)+');');if x=nil then Result:=nil else Result:=new Key(x.id,if ((scenario=17) or (scenario=18)) then x.score else x.score mod 3);end;
function StringKey(x:Item):string;begin Touch('K('+Tag(x)+');');if x=nil then Result:=nil else if x.score<0 then Result:='z' else if x.score=0 then Result:='a' else Result:='A';end;
procedure Run;
begin
 if (scenario=0) or (scenario=17) then Sort(lst);
 if scenario=1 then lst.Sort;
 if scenario=2 then SortDescending(lst);
 if scenario=3 then Sort(lst,CompareFunction);
 if scenario=4 then lst.Sort(CompareFunction);
 if scenario=5 then Sort(lst,Less);
 if scenario=6 then Sort(lst,IntKey);
 if (scenario=7) or (scenario=18) then Sort(lst,ClassKey);
 if scenario=8 then Sort(lst,StringKey);
 if scenario=9 then SortDescending(lst,IntKey);
 if scenario=10 then begin var order:IComparer<Item>:=new Order;lst.Sort(order);end;
 if scenario=11 then lst.Sort(Comparer<Item>.Default);
 if scenario=12 then lst.Sort(Comparer<Item>.Create(CompareFunction));
 if scenario=13 then lst.Sort(if size>1 then 1 else 0,if size>2 then size-2 else 0,new Order);
 if scenario=14 then begin var order:IComparer<Item>;lst.Sort(if size>1 then 1 else 0,if size>2 then size-2 else 0,order);end;
 if scenario=15 then Sort&<Item,string>(lst,StringKey);
 if scenario=16 then begin var order:NamedOrder:=new Order;lst.Sort(order);end;
 if (scenario=19) or (scenario=20) then begin var order:IComparer<Item>;if scenario=20 then order:=new Order;lst.Sort(range_index,range_count,order);end;
end;
procedure Borrow(var n:integer);
begin records.Capacity:=20;records.TrimExcess;n:=7;Write('B',n,';');end;
procedure BorrowCase;
begin var r:RecordData;records:=new List<RecordData>(Arr(r));var cursor:=records.GetEnumerator;Borrow(records[0].n);var next:=cursor.MoveNext;Write(records[0].n,' ',records.Capacity,' ',next,' ',cursor.Current.n);end;
begin
 Readln(scenario,size,pad,mode,nils);
 if scenario=21 then begin BorrowCase;Exit;end;
 if (scenario=19) or (scenario=20) then begin range_index:=pad;range_count:=mode;mode:=0;pad:=0;end;
 lst:=new List<Item>(size+pad);
 for var i:=0 to size-1 do
  if (nils<>0) and ((i+1) mod 5=0) then lst.Add(nil)
  else lst.Add(new Item(i,if ((scenario=17) or (scenario=18)) then (if i<size div 2 then 2*i else 2*(size-i)-1) else (i*17+13) mod 7-3));
 alias:=lst;oldCursor:=lst.GetEnumerator;
 foreach var ignored in new GuardSource do begin Run;Write('DONE;');end;
end.
