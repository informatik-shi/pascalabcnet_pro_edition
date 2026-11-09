var created:integer;

function Next:integer;
begin Inc(created);Result:=created;end;

type Sample=record
  X:=Next;
  Data:=Arr(X,X+1);
end;

begin
  var first:Sample;
  var copy:=first;
  copy.X:=9;
  copy.Data[0]:=88;
  Writeln(first.X,' ',copy.X,' ',first.Data[0],' ',created);

  var values:=new Sample[2];
  SetLength(values,3);
  Writeln(values[0].X,' ',values[1].X,' ',values[2].X,' ',created);

  var empty:=default(Sample);
  Writeln(empty.X,' ',empty.Data=nil,' ',created);
end.
