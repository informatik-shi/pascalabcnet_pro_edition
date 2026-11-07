type
 Item=class(IComparable<Item>)
 public
  id,score:integer;
  constructor Create(i,value:integer);begin id:=i;score:=value;end;
  function CompareTo(other:Item):integer;
  begin Touch('C('+Tag(Self)+','+Tag(other)+');');if mode=4 then Result:=-1 else if other=nil then Result:=1 else if mode=1 then Result:=other.score-score else Result:=score-other.score;end;
 end;
 Key=class(IComparable<Key>)
 public
  id,score:integer;
  constructor Create(i,value:integer);begin id:=i;score:=value;end;
  function CompareTo(other:Key):integer;
  begin Touch('Q('+KeyTag(Self)+','+KeyTag(other)+');');if mode=4 then Result:=-1 else if other=nil then Result:=1 else if mode=1 then Result:=other.score-score else Result:=score-other.score;end;
 end;
var items:array of Item;mode,calls:integer;
function Tag(x:Item):string;begin if x=nil then Result:='nil' else Result:=IntToStr(x.id)+':'+IntToStr(x.score);end;
function KeyTag(x:Key):string;begin if x=nil then Result:='nil' else Result:=IntToStr(x.id)+':'+IntToStr(x.score);end;
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
end;
function Compare(a,b:Item;labelText:string):integer;
begin Touch(labelText+'('+Tag(a)+','+Tag(b)+');');if mode=4 then Result:=-1 else if a=nil then begin if b=nil then Result:=0 else Result:=-1;end else if b=nil then Result:=1 else if mode=1 then Result:=b.score-a.score else Result:=a.score-b.score;end;
function CompareItems(a,b:Item):integer:=Compare(a,b,'F');
function Less(a,b:Item):boolean:=Compare(a,b,'L')<0;
function IntKey(x:Item):integer;begin Touch('K('+Tag(x)+');');if x=nil then Result:=-99 else Result:=x.score mod 3;end;
function ClassKey(x:Item):Key;begin Touch('K('+Tag(x)+');');if x=nil then Result:=nil else Result:=new Key(x.id,if scenario>=12 then x.score else x.score mod 3);end;
function StringKey(x:Item):string;begin Touch('K('+Tag(x)+');');if x=nil then Result:=nil else if x.score<0 then Result:='z' else if x.score=0 then Result:='a' else Result:='A';end;
var scenario,size,seed,nils:integer;
function Killer(i,n:integer):integer;
begin if i<n div 2 then Result:=2*i else Result:=2*(n-i)-1;end;
begin
 Readln(scenario,size,seed,mode,nils);items:=new Item[size];
 for var i:=0 to size-1 do if (nils=0) or ((i+seed) mod 5<>0) then items[i]:=new Item(i,if scenario>=12 then Killer(i,size) else (i*17+seed*13) mod 7-3);
 if scenario=0 then Sort(items);
 if scenario=1 then items.Sort;
 if scenario=2 then SortDescending(items);
 if scenario=3 then Sort(items,CompareItems);
 if scenario=4 then Sort(items,Less);
 if scenario=5 then Sort(items,IntKey);
 if scenario=6 then Sort(items,ClassKey);
 if scenario=7 then Sort(items,StringKey);
 if scenario=8 then SortDescending(items,IntKey);
 if scenario=9 then items.Sort(ClassKey);
 if scenario=10 then Sort&<Item,Key>(items,ClassKey);
 if scenario=11 then Sort&<Item,string>(items,StringKey);
 if scenario=12 then Sort(items);
 if scenario=13 then Sort(items,ClassKey);
 Write('END',Dump);
end.
