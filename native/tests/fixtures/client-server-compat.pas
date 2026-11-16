program ClientServerSource;
uses ClientServer;
procedure SelectPort(var target: integer);
begin
  target := StrToInt(ParamStr(1))
end;
begin
  Writeln(servername,' ',port);
  ClientServer.servername := '127.0.0.1';
  SelectPort(ClientServer.port);
  var port := 2;
  Writeln(port,' ',ClientServer.port > 0);
  var send: procedure(s:string) := ClientServer.SendCommand;
  send('');
  SendCommand(StringOfChar('Ж',65) + '🌍'#0'!');
  var ask: function(s:string):string := ClientServer.SendCommandAndReceiveAnswer;
  var reply := ask('Request Я🌍');
  Writeln(reply.Length, ' ', Ord(reply[1]), ' ', Ord(reply[2]), ' ', Ord(reply[3]));
end.
