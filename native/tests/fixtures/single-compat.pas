uses Collections;
type Point=record x:integer; end;
function MakeChain:SingleNode<integer>;
begin
  var tail:=new SingleNode<integer>(2,nil);
  Result:=new SingleNode<integer>(1,tail);
end;
begin
  var tail:=new SingleNode<integer>(3,nil);
  var head:=new SingleNode<integer>(1,tail);
  head.Next:=new SingleNode<integer>(2,tail);
  var n:=head;
  while n<>nil do begin Write(n.Data); n:=n.Next; end;
  Writeln;
  var alias:=head; alias.Data:=7; head.Next:=tail;
  Writeln(alias=head,' ',head.Data,' ',head.Next.Data);
  head.Next:=head; Writeln(head.Next=head); head.Next:=nil;
  Writeln(head.Next=nil,' ',tail.Data);
  var p:Point; p.x:=7;
  var recordNode:=new SingleNode<Point>(p,nil); p.x:=9;
  var copied:=recordNode.Data; copied.x:=11;
  var stored:=recordNode.Data; Writeln(stored.x,' ',p.x);
  recordNode.Data:=copied; stored:=recordNode.Data; Writeln(stored.x);
  var a:=Arr(1); var arrayNode:=new SingleNode<array of integer>(a,nil);
  a[0]:=8; var b:=arrayNode.Data; Writeln(b[0]);
  var first:=MakeChain; Writeln(first.Data,' ',first.Next.Data);
end.
