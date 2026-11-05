procedure Stop;
begin
  Halt(StrToInt(ParamStr(1)));
  Write('after procedure');
end;
begin
  Write('before');
  if ParamCount=0 then Halt else Stop;
  Write('after main');
end.
