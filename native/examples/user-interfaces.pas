type
  ICounter = interface
    property Value: integer read write;
    function Add(amount: integer): integer;
    procedure Increase(var amount: integer);
  end;
  IDoubleCounter = interface(ICounter)
    function Twice: integer;
  end;
  Counter = class(IDoubleCounter)
  private
    current: integer;
  public
    property Value: integer read current write current;
    function Add(amount: integer): integer;
    begin
      current += amount;
      Result := current;
    end;
    procedure Increase(var amount: integer);
    begin
      amount += current;
    end;
    function Twice: integer := current * 2;
  end;
begin
  var counterObject := new Counter;
  var counter: ICounter := counterObject;
  counter.Value := 5;
  var sameCounter := counter;
  Writeln(sameCounter.Add(3));
  var amount := 2;
  counter.Increase(amount);
  Writeln(amount);
  Writeln(IDoubleCounter(counter).Twice);
  Writeln(TypeName(counter));
end.
