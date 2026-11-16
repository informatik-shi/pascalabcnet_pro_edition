program SocketsSource;
uses Sockets;
begin
  var port:=StrToInt(ParamStr(1));
  var listener:=CreateServerSocket(port);
  var client:=CreateClientSocket;
  Writeln(listener,' ',client,' ',Connect(client,'127.0.0.1',port));
  var accepted:=Sockets.Accept(listener);
  Writeln(accepted);
  SendString(client,'AЯ🌍');
  var received:string;
  ReceiveString(accepted,received);
  Writeln(received.Length,' ',received);
  SendString(client,'second');
  var receiver: procedure(handle:integer;var payload:string):=Sockets.ReceiveString;
  var slots:=new string[1];
  receiver(accepted,slots[0]);
  Writeln(slots[0]);
  SendString(client,StringOfChar('x',2048));
  var total:=0;
  while total<2048 do
  begin
    ReceiveString(accepted,received);
    Assert((received.Length>0) and (received.Length<=1024));
    total+=received.Length;
  end;
  Writeln(total);
  CloseSocket(client);
  ReceiveString(accepted,received);
  Writeln(received.Length);
  CloseSocket(accepted);
  CloseSocket(listener);
  CloseSocket(listener);
  CloseSocket(417);
  var another:=CreateClientSocket;
  Writeln(another);
  CloseSocket(another);
end.
