var target:List<integer>;
var saved,original:array of integer;
var probe:IEnumerator<integer>;
var suppliedCount,capacity,position,failAt,operation,callback:integer;

type Seed=class(ICollection<integer>)
 public
 function CountValue:integer:=capacity;
 property Count:integer read CountValue;
 property IsReadOnly:boolean read boolean(False);
 procedure Add(n:integer);begin end;
 procedure Clear;begin end;
 function Contains(n:integer):boolean:=False;
 function Remove(n:integer):boolean:=False;
 procedure CopyTo(a:array of integer;index:integer);
 begin original:=a;for var i:=0 to capacity-1 do a[index+i]:=i+1;end;
 function GetEnumerator:IEnumerator<integer>:=new List<integer>().GetEnumerator;
end;

type Bag=record(ICollection<integer>)
 public
 function CountValue:integer;
 begin
   Writeln('C');
   if callback=1 then begin target.Add(90);target.RemoveAt(target.Count-1);end;
   if callback=2 then target.Clear;
   if callback=3 then target.Capacity+=7;
   Result:=suppliedCount;
 end;
 property Count:integer read CountValue;
 property IsReadOnly:boolean read boolean(False);
 procedure Add(n:integer);begin end;
 procedure Clear;begin end;
 function Contains(n:integer):boolean:=False;
 function Remove(n:integer):boolean:=False;
 procedure CopyTo(a:array of integer;index:integer);
 begin
   saved:=a;
   if callback=4 then begin target.Add(90);target.RemoveAt(target.Count-1);end;
   if callback=5 then target.Capacity+=7;
   if callback=6 then original[0]:=42;
   Writeln('P ',a.Length,' ',index,' ',target.ToArray,' ',target.Capacity);
   Writeln('A ',original);
   if (callback<>1) and (callback<>2) and (callback<>4) then Writeln(probe.MoveNext,' ',probe.Current);
   for var i:=0 to 1 do begin
     if i=failAt then Write(1 div 0);
     a[index+i]:=7+i;
     Writeln('W ',i,' ',target.ToArray);
     Writeln('A ',original);
   end;
 end;
 function GetEnumerator:IEnumerator<integer>;
 begin Writeln('E');Result:=new List<integer>(Arr(9,10)).GetEnumerator;end;
end;

begin
 Read(suppliedCount,capacity,position,failAt,operation,callback);
 target:=new List<integer>(new Seed);target.RemoveRange(3,capacity-3);
 probe:=target.GetEnumerator;probe.MoveNext;
 var source:ICollection<integer>:=new Bag;
 if operation=0 then target.AddRange(source) else target.InsertRange(position,source);
 Writeln(target.ToArray,' ',target.Count,' ',target.Capacity);
 Writeln('A ',original);
 if saved<>nil then begin Writeln(saved);saved[0]:=99;Writeln(target.ToArray);end;
 Writeln(probe.MoveNext);
end.
