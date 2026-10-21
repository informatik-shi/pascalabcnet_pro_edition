uses Collections;
begin
  var stack := new Stack<integer>;
  stack.Push(10); stack.Push(20); stack.Push(30);
  Write('Стек: ');
  while not stack.IsEmpty do Write(stack.Pop, ' ');
  Writeln;

  var queue := new Queue<string>;
  queue.Enqueue('Анна'); queue.Enqueue('Борис'); queue.Enqueue('Вера');
  Write('Очередь: ');
  while not queue.IsEmpty do Write(queue.Dequeue, ' ');
  Writeln;
  Writeln('Пустая очередь: ', queue.IsEmpty);
end.
