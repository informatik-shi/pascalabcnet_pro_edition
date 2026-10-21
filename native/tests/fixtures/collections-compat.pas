uses Collections;
type Point=record x,y:integer; end;
procedure Add(s:Stack<integer>);
begin s.Push(3); end;
begin
  var s:=new Stack<integer>();
  Writeln(s.IsEmpty);
  s.Push(1); s.Push(2); Add(s);
  Writeln('[',s.ToString,']');
  Writeln(s.Top,' ',s.Top(),' ',s.Pop);
  s.Print;
  s.Println;
  Writeln(s.Pop,' ',s.Pop,' ',s.IsEmpty());
  s.Push(9); Writeln(s.Pop);
  var q:=new Queue<string>;
  Writeln('[',q.ToString,']');
  q.Enqueue('Анна'); q.Enqueue('Борис');
  Writeln(q.Top,' ',q.Top());
  q.Print; Write('|'); q.Println;
  Writeln(q.Dequeue,' ',q.Dequeue,' ',q.IsEmpty);
  q.Enqueue('Вера'); Writeln(q.Dequeue);
  var p:Point; p.x:=7;
  var records:=new Stack<Point>; records.Push(p); p.x:=9;
  var top:=records.Top; top.x:=11;
  var removed:=records.Pop; Writeln(removed.x,' ',p.x);
  var a:=Arr(1,2); var arrays:=new Queue<array of integer>;
  arrays.Enqueue(a); a[0]:=8;
  var b:=arrays.Dequeue; Writeln(b[0],' ',b[1]);
end.
