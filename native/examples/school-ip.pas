uses School;

begin
  var адрес := new Addr32('192.168.1.129');
  Writeln('Адрес: ',адрес.Value);
  Writeln('Число: ',адрес.Value10);
  Writeln('Биты: ',адрес.AddrFormat(адрес.Value2));
  var сеть := new CalcIP(адрес.Value,24);
  Writeln('Маска: ',сеть.Netmask.Value);
  Writeln('Сеть: ',сеть.Network.Value);
  Writeln('Broadcast: ',сеть.Broadcast.Value);
  Writeln('Хостов: ',сеть.Hosts);
  Writeln('Диапазон: ',сеть.Hostmin.Value,' - ',сеть.Hostmax.Value);
  var маленькаяСеть := new CalcIP('10.0.0.1','255.255.255.252');
  foreach var хост in маленькаяСеть.GenAddr do
    Writeln('Хост: ',хост.Value);
end.
