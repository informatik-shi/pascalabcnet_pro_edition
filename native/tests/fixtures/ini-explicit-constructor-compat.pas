program IniConstructors;
uses IniFile;
function NewMarker: integer;
begin
  Writeln('initialize field');
  Result := 17;
end;

function PassName(value: string): string;
begin
  Writeln('constructor argument');
  Result := value;
end;

type
  Settings = class(TIniFile)
    Marker: integer := NewMarker;
    constructor(filename: string);
    begin
      inherited Create(PassName(filename));
      Writeln('after ',FileName,' ',ReadInteger('app','count',-1));
      inherited WriteInteger('app','count',ReadInteger('app','count',0)+1);
    end;
    function ParentCount: integer := inherited ReadInteger('app','count',-1);
  end;

begin
  var x:=new Settings('explicit-constructor.ini');
  Writeln(x.ParentCount,' ',x.Marker);
  x.Save;
  var y:=new Settings('explicit-constructor.ini');
  var baseSettings: TIniFile := y;
  Writeln(y.ParentCount,' ',baseSettings.GetType.Name,' ',y.Marker);
end.
