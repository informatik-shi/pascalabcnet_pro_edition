begin
  Writeln('Завершение программы');
  if ParamCount=0 then Halt
  else Halt(StrToInt(ParamStr(1)));
  Writeln('after Halt');
end.
