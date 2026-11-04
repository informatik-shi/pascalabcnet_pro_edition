// Native compound assignments: types are checked before execution.
uses Collections;

type Counter = class
public
  value: integer := 1;
  property Current: integer read value write value;
end;

begin
  var n := 5;
  n += 2;
  n *= 3;
  n -= 1;
  var r := 9.0;
  r /= 2;
  Writeln(n, ' ', r);
  var text := 'Pascal';
  text += ' ';
  text += 42;
  text *= 2;
  Writeln(text);
  var big := 100000000000000000000bi;
  big += 2;
  big *= 3;
  Writeln(big);
  var values := Arr(1, 2, 3);
  values[1] += 10;
  Writeln(values);
  var list := Lst(1);
  list += 2;
  list += Lst(3, 4);
  Writeln(list);
  Writeln(list * 2);
  var queue := new Queue<integer>;
  queue += 10;
  queue += 20;
  Writeln(queue.Dequeue, ' ', queue.Dequeue);
  var counter := new Counter;
  counter.Current += 4;
  Writeln(counter.Current);
end.
