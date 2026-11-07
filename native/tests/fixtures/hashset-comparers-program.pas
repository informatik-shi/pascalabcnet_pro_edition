type
 Order=class(IEqualityComparer<integer>) public labelText:string;modulus:integer;constructor Create(l:string;m:integer);begin labelText:=l;modulus:=m;end;
 function Equals(a,b:integer):boolean;begin var changed:=Touch(labelText+'E('+IntToStr(a)+','+IntToStr(b)+');',False);Result:=changed and ((mutation=3) or (mutation=9)) or (a mod modulus=b mod modulus);end;
 function GetHashCode(a:integer):integer;begin Touch(labelText+'H('+IntToStr(a)+');',True);Result:=-2147483647;end;end;
 GuardCursor=class(IEnumerator<integer>) private n:integer;public function MoveNext:boolean;begin n+=1;Result:=n=1;end;property Current:integer read Zero;function Zero:integer:=0;procedure Reset;begin n:=0;end;procedure Dispose;begin Finish;end;end;
 GuardSource=class(IEnumerable<integer>) public function GetEnumerator:IEnumerator<integer>:=new GuardCursor;end;
var set1,alias1,resultSet,otherSet:HashSet<integer>;oldCursor:IEnumerator<integer>;policy,alternate:IEqualityComparer<integer>;other:sequence of integer;mutation,trigger,phase,hashCalls,equalsCalls:integer;active,busy:boolean;
function Touch(text:string;hashing:boolean):boolean;
begin Result:=False;if not active or busy then exit;Write(text);if hashing then hashCalls+=1 else equalsCalls+=1;
 if (mutation=0) or ((if hashing then 0 else 1)<>phase) or ((if hashing then hashCalls else equalsCalls)<>trigger) then exit;
 busy:=True;Write('M',mutation,';');if mutation=1 then begin var zero:=0;var q:=1 div zero;end;if mutation=2 then begin var a:=Arr(1);var q:=a[999];end;
 if mutation=3 then set1.Clear;if mutation=4 then for var i:=20 to 39 do set1.Add(i);if mutation=5 then set1.TrimExcess;if mutation=6 then set1.Remove(2);if mutation=7 then set1:=new HashSet<integer>(set1,policy);if mutation=8 then set1:=nil;if mutation=9 then begin set1.Clear;set1.Add(99);end;if mutation=10 then set1.Remove(4);busy:=False;Result:=True;
end;
function Dump(s:HashSet<integer>):string;
begin if s=nil then exit('nil');Result:='[';var first:=True;foreach var x in s do begin if not first then Result+=',';first:=False;Result+=IntToStr(x);end;Result+=']';end;
function Label(s:HashSet<integer>):string;
begin if s=nil then exit('nil');if s.Comparer=policy then exit('P');if s.Comparer=alternate then exit('A');if s.Comparer=EqualityComparer<integer>.Default then exit('D');Result:='?';end;
procedure Finish;
begin active:=False;Write('Z',Dump(set1),':',if set1=nil then -1 else set1.Count,':',Label(set1),'|',Dump(alias1),':',alias1.Count,'|',Dump(resultSet),':',Label(resultSet),'|E');var n:=oldCursor.MoveNext;Write(n);if n then Write(':',oldCursor.Current);Write(';');end;
function Setup(shape:integer;cmp:IEqualityComparer<integer>):HashSet<integer>;
begin var s:=if shape=1 then new HashSet<integer>(new integer[0],cmp) else new HashSet<integer>(cmp);if shape=2 then begin s.Add(9);s.Clear;end;if shape=3 then foreach var n in Arr(0,1,2,4,6,8) do s.Add(n);if shape=4 then begin for var i:=0 to 79 do s.Add(i);for var i:=0 to 74 do s.Remove(i);end;if shape=5 then begin for var i:=0 to 16 do s.Add(i);foreach var i in Arr(1,3,5,7) do s.Remove(i);end;Result:=s;end;
function Predicate(n:integer):boolean;begin Touch('P('+IntToStr(n)+');',False);Result:=n mod 2=0;end;
procedure Run(op:integer);
begin
 if op=0 then Write(set1.Contains(4));if op=1 then Write(set1.Add(4));if op=2 then Write(set1.Remove(4));if op=3 then begin var value:integer;var found:=set1.TryGetValue(4,value);Write(found,':',value);end;
 if op=4 then set1.Clear;if op=5 then set1.TrimExcess;if op=6 then resultSet:=new HashSet<integer>(set1);if op=7 then resultSet:=new HashSet<integer>(set1,policy);if op=8 then resultSet:=new HashSet<integer>(set1,alternate);
 if op=9 then set1.UnionWith(other);if op=10 then set1.ExceptWith(other);if op=11 then set1.IntersectWith(other);if op=12 then set1.SymmetricExceptWith(other);if op=13 then Write(set1.SetEquals(other));if op=14 then Write(set1.IsSubsetOf(other));if op=15 then Write(set1.IsProperSubsetOf(other));if op=16 then Write(set1.IsSupersetOf(other));if op=17 then Write(set1.IsProperSupersetOf(other));if op=18 then Write(set1.Overlaps(other));if op=19 then Write(set1.RemoveWhere(Predicate));if op=20 then resultSet:=Copy(set1);if op=21 then resultSet:=set1+otherSet;if op=22 then resultSet:=set1-otherSet;if op=23 then resultSet:=set1*otherSet;if op=24 then resultSet:=new HashSet<integer>(other,policy);if op=25 then Write(44 in set1);if op=26 then begin var a:=new integer[alias1.Count+2];set1.CopyTo(a,1);Write(a);end;if op=27 then begin set1.Remove(4);set1.Add(44);set1.Add(4);end;Write('DONE;');
end;
begin
 var op,shape,p,kind:integer;Readln(op,shape,p,kind,mutation,phase,trigger);if p=0 then policy:=EqualityComparer<integer>.Default else policy:=new Order('U',if p=1 then 3 else if p=2 then 1000 else 2);alternate:=new Order('V',2);set1:=Setup(shape,policy);alias1:=set1;
 var data:=Arr(1,4,4,7,2,8);if kind=1 then otherSet:=new HashSet<integer>(data,policy) else if kind=2 then begin var cmp:IEqualityComparer<integer>;if p=0 then cmp:=EqualityComparer<integer>.Default else cmp:=new Order('U',if p=1 then 3 else if p=2 then 1000 else 2);otherSet:=new HashSet<integer>(data,cmp);end else otherSet:=new HashSet<integer>(data,alternate);if kind=4 then otherSet:=set1;if kind=7 then begin otherSet:=new HashSet<integer>(policy);for var i:=0 to 20 do otherSet.Add(i);end;
 if kind=0 then other:=data else if kind=5 then other:=nil else if kind=6 then other:=new integer[0] else other:=otherSet;oldCursor:=set1.GetEnumerator;active:=True;foreach var ignored in new GuardSource do Run(op);
end.
