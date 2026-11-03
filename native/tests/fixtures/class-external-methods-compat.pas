type
  Counter = class
  private
    current: integer;
    function GetValue: integer;
  public
    constructor Create(initial: integer);
    procedure Add(step: integer := 1);
    property Value: integer read GetValue;
  end;

constructor Counter.Create(initial: integer);
begin
  current := initial;
end;

procedure Counter.Add(step: integer);
begin
  current := current + step;
end;

function Counter.GetValue: integer;
begin
  Result := current;
end;

begin
  var counter := new Counter(3);
  counter.Add;
  counter.Add(2);
  Writeln('Счётчик: ', counter.Value);
end.
