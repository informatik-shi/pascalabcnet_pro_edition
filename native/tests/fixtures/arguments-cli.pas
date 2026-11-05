begin
  Writeln(ParamCount);
  for var i:=1 to ParamCount do
    Writeln('[',ParamStr(i),']');
  Writeln(ParamStr(0)=GetEXEFileName);
end.
