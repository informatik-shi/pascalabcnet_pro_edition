program VirtualDispatch;
type
  StateProbe = interface
    function Score: integer;
  end;
  RootState = class(StateProbe)
  public
    CurrentValue: integer;
    function Score: integer; virtual;
    begin
      Result := 1;
    end;
    function ThroughRoot: integer := Score;
    procedure Update(value: integer); virtual;
    begin
      CurrentValue := value;
    end;
    property SelectedScore: integer read Score;
    property Current: integer read CurrentValue write Update;
  end;
  MidState = class(RootState)
  public
    function Score: integer; override;
    begin
      Result := inherited Score + 10;
    end;
  end;
  LeafState = class(MidState)
  public
    function Score: integer; override;
    begin
      Result := inherited Score + 100;
    end;
    procedure Update(value: integer); override;
    begin
      inherited Update(value+100);
    end;
  end;
  HiddenState = class(LeafState)
  public
    function Score: integer; virtual;
    begin
      Result := 1000;
    end;
  end;
  HiddenLeaf = class(HiddenState)
  public
    function Score: integer; override;
    begin
      Result := inherited Score + 200;
    end;
  end;
begin
  var leaf:=new LeafState;
  var root:RootState:=leaf;
  var mid:MidState:=leaf;
  var probe:StateProbe:=leaf;
  Writeln(leaf.Score,' ',root.Score,' ',mid.Score,' ',root.ThroughRoot,' ',probe.Score,' ',root.SelectedScore);
  root.Current:=7;
  Writeln(root.Current,' ',leaf.CurrentValue);
  var hidden:=new HiddenLeaf;
  var baseHidden:RootState:=hidden;
  var shadow:HiddenState:=hidden;
  var hiddenProbe:StateProbe:=hidden;
  Writeln(hidden.Score,' ',baseHidden.Score,' ',shadow.Score,' ',baseHidden.ThroughRoot,' ',hiddenProbe.Score,' ',baseHidden.SelectedScore);
end.
