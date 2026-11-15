program ClassObjectRefs;
uses IniFile;
type
  Widget = class
  public
    Number: integer;
    function ToString: string; override; begin Result:='widget'; end;
  end;
  LabelWidget = class(Widget)
  public
    function ToString: string; override; begin Result:='label'; end;
  end;
  OtherWidget = class(Widget)
  end;
  Plain = class
  private
    Hidden: integer;
  public
    Visible: integer;
    Next: Plain;
  end;
  Config = class(TIniFile)
  public
    function ToString: string; override; begin Result:='cfg'; end;
  end;

begin
  var actual := new LabelWidget;
  var base: Widget := actual;
  var boxed: object := base;
  var second := object(actual);
  Writeln(boxed = second, ' ', boxed.GetType.FullName, ' ', boxed.ToString);
  Writeln(Widget(boxed).ToString, ' ', LabelWidget(boxed).ToString);
  Widget(boxed).Number := 17;
  Writeln(actual.Number, ' ', ObjectToString(boxed));
  var missing: Widget := nil;
  boxed:=missing;
  Writeln(boxed = nil, ' ', Widget(boxed) = nil);
  var cfg: TIniFile := new Config('missing.ini');
  boxed:=cfg;
  Writeln(boxed.GetType.FullName, ' ', TIniFile(boxed).ToString, ' ', boxed.ToString);
  var defaultCfg := new TIniFile('missing.ini');
  Writeln('<',ObjectToString(defaultCfg),'>');
  var defaultLabel := new OtherWidget;
  Writeln('<',ObjectToString(defaultLabel),'>');
  var plainValue := new Plain;
  plainValue.Visible:=5;
  plainValue.Next := plainValue;
  Writeln(ObjectToString(plainValue));
  var renderedItems := new List<object>;
  renderedItems.Add(actual);
  renderedItems.Add(defaultCfg);
  renderedItems.Add(plainValue);
  Writeln(ObjectToString(renderedItems));
  var keys := new Dictionary<object,integer>;
  keys.Add(second,117);
  Writeln(keys[object(base)]);
end.
