uses Collections;
type Point=record x:integer; end;
function Head:LinkedListNode<integer>;
begin
  var l:=new LinkedList<integer>; l.AddLast(1); l.AddLast(2); Result:=l.First;
end;
begin
  var l:=new LinkedList<integer>;
  Writeln(l.First=nil,' ',l.Last=nil);
  l.AddLast(1); l.AddLast(3); l.AddBefore(l.Last,2); l.AddAfter(l.Last,4);
  l.AddBefore(l.First,0); l.AddAfter(l.First.Next,8);
  var n:=l.First;
  while n<>nil do begin Write(n.Value); n:=n.Next; end;
  Write(' '); n:=l.Last;
  while n<>nil do begin Write(n.Value); n:=n.Prev; end;
  Writeln;
  var removed:=l.First.Next.Next; l.Remove(removed);
  Writeln(removed.Prev.Value,' ',removed.Next.Value,' ',l.First.Next.Next.Value);
  l.RemoveFirst; l.RemoveLast;
  Writeln('[',l.ToString,']'); l.Print; Write('|'); l.Println;
  var a:=new LinkedListNode<integer>(7,nil,nil);
  var b:=new LinkedListNode<integer>(8,a,nil);
  b.Value:=9; Writeln(b.Prev=a,' ',a.Next=nil,' ',b.Value);
  var p:Point; p.x:=7; var points:=new LinkedList<Point>; points.AddLast(p);
  p.x:=9; var copied:=points.First.Value; copied.x:=11;
  var stored:=points.First.Value; Writeln(stored.x,' ',p.x);
  points.First.Value:=copied; stored:=points.First.Value; Writeln(stored.x);
  var h:=Head; Writeln(h.Value,' ',h.Next.Value);
  var old:=l.First; l.RemoveFirst;
  Writeln(old.Value,' ',old.Next.Value,' ',l.First.Prev=nil);
end.
