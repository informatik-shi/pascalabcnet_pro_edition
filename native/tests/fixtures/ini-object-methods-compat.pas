uses IniFile;

var calls: integer;

function Other: integer;
begin
  Inc(calls);
  Result := 42;
end;

begin
  var ini := new TIniFile('methods.ini');
  var separate := new TIniFile('methods.ini');
  var alias := ini;
  Writeln(ini.ToString, ' ', ini.GetType.FullName);
  Writeln(ini.Equals(alias), ' ', ini.Equals(separate), ' ', ini.Equals(nil),
    ' ', ini.Equals(Other()), ' ', calls);
  var before := ini.GetHashCode;
  ini.WriteInteger('test', 'value', 3);
  Writeln(before=alias.GetHashCode, ' ', ini.Equals(alias));
end.
