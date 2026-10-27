uses School;
begin
  var a := new Addr32('192.168.1.129');
  var m := new Addr32(24);
  Writeln(a.Value10,' ',a.Value2);
  Writeln((a and m).Value,' ',(a or not m).Value,' ',(a+1).Value);
  var alias := a;
  alias.Value := '001.002.003.004';
  Writeln(a.Value,' ',a.Value10,' ',a=alias);
  var c := new CalcIP('10.0.0.1','255.255.255.252');
  Writeln(c.Bitmask,' ',c.Hosts,' ',c.Network.Value,' ',c.Broadcast.Value);
  foreach var host in c.GenAddr do Writeln(host.Value);
  foreach var bits in c.GenAddrBin do Writeln(bits);
  c.Address.Value := '10.1.2.3';
  Writeln(c.Address.Value,' ',c.Network.Value);
  var p := new CalcIP('10.0.0.2',31);
  Writeln(p.Hosts,' ',p.Hostmin.Value,' ',p.Hostmax.Value);
  Writeln(p.GenAddr.Length,' ',p.GenAddrBin.Length);
end.
