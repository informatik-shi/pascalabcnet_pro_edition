begin
  Writeln(PascalABCVersion);
  Writeln(ParamCount);
  for var i:=1 to ParamCount do
    Writeln('[',ParamStr(i),']');
  Writeln(ParamStr(0)=GetEXEFileName);
  Writeln(FileExists(GetEXEFileName));
  var total:int64:=DiskSize(GetDir);
  var free:int64:=DiskFree(GetDir);
  Writeln((free>=0) and (total>=free));
  Writeln(DiskFree(nil),' ',DiskSize(''));
end.
