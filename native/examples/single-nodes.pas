uses Collections;
begin
  var tail := new SingleNode<string>('Вера',nil);
  var head := new SingleNode<string>('Анна',tail);
  head.Next := new SingleNode<string>('Борис',tail);
  Write('Цепочка: ');
  var node := head;
  while node <> nil do begin
    Write(node.Data,' '); node := node.Next;
  end;
  Writeln;
  head.Next := tail;
  tail.Data := 'Виктория';
  Writeln('После изменения: ',head.Data,' ',head.Next.Data);
end.
