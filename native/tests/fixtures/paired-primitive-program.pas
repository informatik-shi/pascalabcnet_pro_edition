type IntOrder=class(IComparer<integer>) public function Compare(a,b:integer):integer:=IntCompare(a,b);end;
 GuardCursor=class(IEnumerator<integer>)
 private n:integer;
 public function MoveNext:boolean;begin n+=1;Result:=n=1;end;
  property Current:integer read Zero;function Zero:integer:=0;
  procedure Reset;begin n:=0;end;
  procedure Dispose;begin Finish;end;
 end;
 GuardSource=class(IEnumerable<integer>) public function GetEnumerator:IEnumerator<integer>:=new GuardCursor;end;
var scenario,size,kind,seed,start,length:integer;keys,values:array of integer;strings:array of string;
function IntCompare(a,b:integer):integer;begin Write('I(',a,',',b,');');Result:=a-b;end;
function IntegerDump(a:array of integer):string;
begin if a=nil then begin Result:='nil';Exit;end;Result:='[';for var i:=0 to a.Length-1 do begin if i<>0 then Result+=',';Result+=IntToStr(a[i]);end;Result+=']';end;
function StringDump(a:array of string):string;
begin if a=nil then begin Result:='nil';Exit;end;Result:='[';for var i:=0 to a.Length-1 do begin if i<>0 then Result+=',';Result+=a[i];end;Result+=']';end;
procedure Finish;
begin Write('|',IntegerDump(keys),' ',if (kind=3) or (kind=4) then StringDump(strings) else IntegerDump(values));end;
procedure SortIntegers;
begin
 if scenario=0 then Array.Sort(keys,values);
 if scenario=1 then Array.Sort(keys,values,Comparer<integer>.Default);
 if scenario=2 then Array.Sort(keys,values,Comparer<integer>.Create(IntCompare));
 if scenario=3 then Array.Sort(keys,values,new IntOrder);
 if scenario=4 then Array.Sort(keys,values,start,length);
 if scenario=5 then Array.Sort(keys,values,start,length,nil);
 if scenario=6 then Array.Sort(keys,values,start,length,new IntOrder);
end;
procedure SortStrings;
begin
 if scenario=0 then Array.Sort(keys,strings);
 if scenario=1 then Array.Sort(keys,strings,Comparer<integer>.Default);
 if scenario=2 then Array.Sort(keys,strings,Comparer<integer>.Create(IntCompare));
 if scenario=3 then Array.Sort(keys,strings,new IntOrder);
 if scenario=4 then Array.Sort(keys,strings,start,length);
 if scenario=5 then Array.Sort(keys,strings,start,length,nil);
 if scenario=6 then Array.Sort(keys,strings,start,length,new IntOrder);
end;
begin
 Readln(scenario,size,kind,seed);keys:=new integer[size];
 for var i:=0 to size-1 do keys[i]:=if size=1024 then (if i<size div 2 then 2*i else 2*(size-i)-1) else (i*17+seed*13) mod 7-3;
 values:=new integer[if kind=5 then size+3 else size];for var i:=0 to values.Length-1 do values[i]:=100+i;
 if kind=1 then values:=keys;if kind=2 then values:=nil;
 strings:=new string[size];for var i:=0 to size-1 do strings[i]:='v'+IntToStr(i);if kind=4 then strings:=nil;
 start:=if size=1024 then 0 else if size>1 then 1 else 0;length:=if size=1024 then size else Max(0,size-2);
 foreach var ignored in new GuardSource do begin if (kind=3) or (kind=4) then SortStrings else SortIntegers;end;
end.
