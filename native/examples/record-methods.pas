type ICounter=interface
  property Value:integer read write;
  procedure Add(n:integer);
  procedure Reset(n:integer);
end;
Counter=record(ICounter)
  private number:integer;
  public constructor(n:integer);begin number:=n;end;
  property Value:integer read number write number;
  procedure Add(n:integer);begin number+=n;end;
  procedure Reset(n:integer);begin Self:=new Counter(n);end;
end;
begin
  var original:=new Counter(3);
  var copied:=original;
  original.Add(2);
  Writeln(original.Value,' ',copied.Value);
  var boxed:ICounter:=original;
  var same:=boxed;
  boxed.Add(10);
  Writeln(original.Value,' ',boxed.Value,' ',same.Value);
  boxed.Reset(7);
  Writeln(same.Value);
  var restored:=Counter(boxed);
  restored.Add(1);
  Writeln(restored.Value,' ',boxed.Value);
end.
