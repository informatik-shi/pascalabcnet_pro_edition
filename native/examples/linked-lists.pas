uses Collections;
begin
  var names := new LinkedList<string>;
  names.AddLast('Анна'); names.AddLast('Вера');
  names.AddBefore(names.Last,'Борис');
  Write('Вперёд: ');
  var node := names.First;
  while node <> nil do begin
    Write(node.Value,' '); node := node.Next;
  end;
  Writeln;
  names.Remove(names.First.Next);
  Write('Назад после удаления: ');
  node := names.Last;
  while node <> nil do begin
    Write(node.Value,' '); node := node.Prev;
  end;
  Writeln;
end.
