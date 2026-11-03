type
  Node = class
    value: integer;
    next: Node;
  end;
begin
  var a := new Node;
  var b := new Node;
  a.value := 7;
  b.value := 9;
  a.next := b;
  b.next := a;
  Writeln('Связанные объекты: ', a.next.value, ' ', b.next.value);
  var alias := a;
  alias.value := 11;
  Writeln('Общая ссылка: ', a.value);
  var nodes := Lst(a, b);
  foreach var node in nodes do
    Write(node.value, ' ');
  Writeln;
end.
