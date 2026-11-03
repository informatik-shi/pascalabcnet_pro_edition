type
  Counter = class
  private
    current: integer := 1;
  public
    constructor(value: integer) := current := current + value;
    procedure Add(value: integer := 1) := current := current + value;
    function Value: integer := current;
    function Mapper: integer -> integer := x -> x + current;
  end;

function Twice(value: integer): integer := value * 2;

begin
  var counter := new Counter(2);
  counter.Add;
  var mapper := counter.Mapper;
  Writeln('Значение: ', Twice(counter.Value));
  counter.Add(3);
  Writeln('Замыкание: ', mapper(1));
end.
