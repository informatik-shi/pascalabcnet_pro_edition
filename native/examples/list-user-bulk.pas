var saved:array of integer;
type Batch=record(ICollection<integer>)
 public
 function CountValue:integer;begin Writeln('Count');Result:=2;end;
 property Count:integer read CountValue;
 property IsReadOnly:boolean read boolean(False);
 procedure Add(n:integer);begin end;
 procedure Clear;begin end;
 function Contains(n:integer):boolean:=False;
 function Remove(n:integer):boolean:=False;
 procedure CopyTo(a:array of integer;index:integer);
 begin
  Writeln('CopyTo');saved:=a;a[index]:=7;
  a[index+1]:=8;
 end;
 function GetEnumerator:IEnumerator<integer>:=new List<integer>().GetEnumerator;
end;
begin
 var values:=Lst(1,2,3);
 var batch:ICollection<integer>:=new Batch;
 values.InsertRange(1,batch);Writeln(values.ToArray);
 saved[0]:=99;
 Writeln(values.ToArray);Writeln(saved);
end.
