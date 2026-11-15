program IniDerived;
uses IniFile;
type
  Settings = class(IniFile.TIniFile)
    Marker: integer;
    procedure SetMarker(n: integer) := Marker := n;
    function Port(orElse: integer): integer := ReadInteger('server','port',orElse);
  end;
  ServerSettings = class(Settings)
    Limit: integer;
  end;
begin
  var path: string := 'ini-derived-fixture.ini';
  var config := new ServerSettings(path);
  Writeln(config.FileName,' ',config.Port(80));
  config.WriteInteger('server','port',443);
  config.WriteBoolean('server','trace',true);
  config.WriteString('server','host','local');
  var root: TIniFile := config;
  Writeln(root.GetType.Name,' ',root.ToString,' ',root.Equals(config));
  Writeln(root.ReadInteger('server','port',0),' ',root.ReadBoolean('server','trace',false),' ',root.ReadString('server','host','missing'));
  root.Save;
  var loaded := new Settings(path);
  Writeln(loaded.Port(0),' ',loaded.FileName,' ',loaded.GetType.Name);
  var uninitialized := new ServerSettings;
  Writeln(uninitialized.GetType.Name,' ',uninitialized.FileName=nil);
  var down := Settings(root);
  config.SetMarker(41);
  config.Limit := 42;
  Writeln(down.Port(0),' ',down.GetType.Name,' ',down=loaded);
  Writeln(down.Marker,' ',config.Marker,' ',config.Limit);
end.
