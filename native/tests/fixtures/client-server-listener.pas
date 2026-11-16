program ClientServerListenerSource;
uses ClientServer;
function Answer(s:string): string;
begin
  if s='stop' then Halt(0);
  Result:=s+#0'Ответ🌍'
end;
begin
  ClientServer.port:=StrToInt(ParamStr(1));
  if ParamStr(2)='reply' then
  begin
    var run: procedure(callback: string -> string) := ClientServer.RunServer;
    run(Answer)
  end
  else
  begin
    var seen:=0;
    RunServer(procedure(s:string) -> begin
      if s='stop' then Halt(0);
      seen+=s.Length;
      Writeln(seen,' ',s)
    end);
  end;
end.
