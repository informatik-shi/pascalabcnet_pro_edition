program CollectionTextCallbacks;
uses Collections;
type
  Item = class
    LabelText: string;
    Action: integer;
    constructor(text: string; act: integer);
    begin
      LabelText:=text;
      Action:=act;
    end;
  public
    function ToString: string; override;
  end;

var queue: Queue<Item>;
var stack: Stack<Item>;

function Item.ToString: string;
begin
  var old := Action;
  Action:=0;
  case old of
    1: queue.Enqueue(new Item('added',0));
    2: begin
      queue.Dequeue;
      queue.Dequeue;
      queue.Enqueue(new Item('fresh',0));
    end;
    3: stack.Push(new Item('extra',0));
    4: begin
      queue.Enqueue(new Item('linked',0));
      queue.Dequeue;
      queue.Dequeue;
    end;
    5: begin
      queue.Enqueue(new Item('nested',0));
      Writeln('[',queue.ToString,']');
    end;
    6: begin
      stack.Pop;
      stack.Pop;
      stack.Push(new Item('new top',0));
    end;
    7: begin
      Result:=nil;
      exit;
    end;
  end;
  Result:=LabelText;
end;

begin
  queue:=new Queue<Item>;
  queue.Enqueue(new Item('first',1));
  queue.Enqueue(new Item('tail',0));
  Writeln('<',queue.ToString,'>');
  queue:=new Queue<Item>;
  queue.Enqueue(new Item('old head',2));
  queue.Enqueue(new Item('old tail',0));
  Writeln('<',queue.ToString,'> <',queue.ToString,'>');
  queue:=new Queue<Item>;
  queue.Enqueue(new Item('prefix',4));
  queue.Enqueue(new Item('middle',0));
  Writeln('<',queue.ToString,'> <',queue.ToString,'>');
  stack:=new Stack<Item>;
  stack.Push(new Item('bottom',0));
  stack.Push(new Item('top',3));
  Writeln('<',stack.ToString,'> <',stack.ToString,'>');
  queue:=new Queue<Item>;
  queue.Enqueue(new Item('outer',5));
  Writeln('<',queue.ToString,'>');
  stack:=new Stack<Item>;
  stack.Push(new Item('hidden tail',0));
  stack.Push(new Item('old top',6));
  Writeln('<',stack.ToString,'> <',stack.ToString,'>');
  stack.Push(new Item('nil result',7));
  Writeln('<',stack.ToString,'>');
end.
