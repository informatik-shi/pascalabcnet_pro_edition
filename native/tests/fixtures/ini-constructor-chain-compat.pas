program IniConstructorChain;
uses IniFile;
type
  BaseSettings = class(TIniFile)
    Number: integer := 17;
    constructor(filename: string; defaultValue: integer := 5);
    begin
      inherited Create(filename);
      WriteInteger('section','default',defaultValue+Number);
      Writeln('base ',Self.GetType.Name,' ',Number);
    end;
  end;
  InheritedSettings = class(BaseSettings)
    ParentFlag: boolean;
    OwnMarker: integer := 4;
  end;
  ExplicitSettings = class(BaseSettings)
    TextMarker: string := 'ready';
    constructor(filename: string);
    begin
      inherited Create(filename,11);
      Writeln('child ',TextMarker);
    end;
  end;
  MiddleSettings = class(BaseSettings)
    MiddleMarker: integer := 7;
  end;
  ViaMiddle = class(MiddleSettings)
    constructor(filename: string);
    begin
      inherited Create(filename,21);
    end;
  end;
begin
  var inheritedOne:=new InheritedSettings('chain-inherited.ini');
  Writeln(inheritedOne.ReadInteger('section','default',0),' ',inheritedOne.ParentFlag,' ',inheritedOne.OwnMarker);
  var explicitOne:=new ExplicitSettings('chain-explicit.ini');
  Writeln(explicitOne.ReadInteger('section','default',0),' ',explicitOne.TextMarker,' ',explicitOne.Number);
  var unassigned:=new InheritedSettings;
  Writeln(unassigned.FileName.Length,' ',unassigned.Number,' ',unassigned.OwnMarker);
  var skipped:=new ViaMiddle('chain-via-middle.ini');
  Writeln(skipped.ReadInteger('section','default',0),' ',skipped.MiddleMarker);
  var asBase: BaseSettings := explicitOne;
  Writeln(asBase.GetType.Name,' ',asBase.Equals(explicitOne));
end.
