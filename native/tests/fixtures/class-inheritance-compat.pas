program ClassInheritance;
function Initial(labelText: string; number: integer): integer;
begin
  Writeln(labelText);
  Result:=number;
end;

type
  StateProbe = interface
    function ReadValue: integer;
  end;
  RootState = class(StateProbe)
    Value: integer := Initial('root field',8);
    constructor(number: integer);
    begin
      Value:=number;
      Writeln('root body ',Self.GetType.Name,' ',Value);
    end;
  public
    function ReadValue: integer := Value;
  end;
  ImplicitState = class(RootState)
    Flag: integer := Initial('implicit field',1);
    constructor;
    begin
      Writeln('implicit body ',ReadValue+Flag);
    end;
  end;
  ExplicitState = class(RootState)
    Marker: integer := Initial('explicit field',2);
    constructor(number: integer);
    begin
      inherited Create(number);
      Writeln('explicit body ',Marker);
    end;
    function ReadValue: integer := inherited ReadValue+Marker;
  end;
  PlainState = class(RootState)
  end;
  EmptyRoot = class
    constructor;
    begin
      inherited Create;
      Writeln(Self.GetType.Name);
    end;
  end;
begin
  var implicitOne:=new ImplicitState;
  var explicitOne:=new ExplicitState(5);
  var root:RootState:=explicitOne;
  Writeln(root.GetType.Name,' ',root.ReadValue,' ',ExplicitState(root).ReadValue);
  var probe: StateProbe := explicitOne;
  Writeln(probe.ReadValue,' ',ExplicitState(probe).Marker);
  var plain:=new PlainState(21);
  Writeln(plain.ReadValue,' ',plain.GetType.Name);
  var defaults:=new PlainState;
  Writeln(defaults.Value,' ',defaults.GetType.Name);
  var rootDefaults:=new RootState;
  Writeln(rootDefaults.Value,' ',rootDefaults.GetType.Name);
  var empty:=new EmptyRoot;
end.
