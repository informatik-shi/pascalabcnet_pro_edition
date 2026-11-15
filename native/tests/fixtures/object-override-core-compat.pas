program UserObjectText;
uses Oberon00System, IniFile;
type
  DefaultLabel = class
  end;
  CustomLabel = class(DefaultLabel)
  public
    function ToString: string; override;
    begin
      Result := '['+inherited ToString+']';
    end;
  end;
  HiddenLabel = class(CustomLabel)
  public
    function ToString: string; virtual;
    begin
      Result := 'hidden';
    end;
  end;
  EmptyLabel = class(DefaultLabel)
  public
    function ToString: string; override;
    begin
      Result := nil;
    end;
  end;
  ConfigLabel = class(TIniFile)
  public
    function ToString: string; override;
    begin
      Result := inherited ToString + ':cfg';
    end;
  end;
begin
  var plain: DefaultLabel := new CustomLabel;
  var hidden := new HiddenLabel;
  var up: DefaultLabel := hidden;
  Writeln(plain.ToString,' ',hidden.ToString,' ',up.ToString);
  Oberon00System.Println(plain);
  Oberon00System.Println(hidden);
  var empty: DefaultLabel := new EmptyLabel;
  Writeln(empty.ToString=nil);
  Oberon00System.Println(empty);
  var ini: TIniFile := new ConfigLabel;
  Writeln(ini.ToString);
end.
