program AbstractDispatch;
type
  Measure = interface
    function Value: integer;
  end;
  AbstractValue = abstract class(Measure)
  public
    function Value: integer; abstract;
    function Twice: integer := Value*2;
  end;
  AbstractMiddle = abstract class(AbstractValue)
  public
    function Value: integer; override; abstract;
  end;
  FixedValue = class(AbstractMiddle)
    Data: integer;
    constructor(number: integer);
    begin
      Data := number;
    end;
  public
    function Value: integer; override;
    begin
      Result := Data;
    end;
  end;
  ImplicitBase = class
    procedure Output; virtual; abstract;
  end;
  OutputValue = class(ImplicitBase)
    procedure Output; override;
    begin
      Writeln('concrete output');
    end;
  end;
begin
  var concreteValue := new FixedValue(9);
  var baseValue: AbstractValue := concreteValue;
  var middle: AbstractMiddle := concreteValue;
  var via: Measure := concreteValue;
  Writeln(baseValue.Value,' ',middle.Value,' ',via.Value,' ',baseValue.Twice);
  Writeln(baseValue.GetType.Name,' ',typeof(AbstractValue).Name);
  var emitter: ImplicitBase := new OutputValue;
  emitter.Output;
end.
