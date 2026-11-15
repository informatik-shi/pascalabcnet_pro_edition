uses System;
begin
  Serialize('native-decimal.nrbf',new Decimal($12345678,$1234567,$1234,True,byte(6)));
  Serialize('native-datetime.nrbf',new DateTime(638448087091234567,DateTimeKind.Utc));
  Serialize('native-timespan.nrbf',TimeSpan.FromTicks(-1234567890123));
  Serialize('native-complex.nrbf',Cplx(-1.25,3.5));
  Writeln(Decimal.GetBits(Decimal(Deserialize('native-decimal.nrbf'))));
  Writeln(DateTime(Deserialize('native-datetime.nrbf')).ToString('O'));
  Writeln(TimeSpan(Deserialize('native-timespan.nrbf')).Ticks);
  Writeln(Complex(Deserialize('native-complex.nrbf')));
  var stream: file;
  Assign(stream,'native-records-stream.nrbf');
  Rewrite(stream);
  stream.Serialize(Decimal.Parse('1.2300'));
  stream.Serialize(Cplx(1.25,-2.5));
  Close(stream);
  Reset(stream);
  var first:=stream.Deserialize;
  Writeln(first.GetType.FullName,' ',Decimal(first));
  Writeln(Complex(stream.Deserialize));
  Close(stream);

  foreach var name in Arr('native-decimal.nrbf','native-datetime.nrbf','native-timespan.nrbf','native-complex.nrbf') do
  begin
    var raw: file;
    Assign(raw,name);
    Reset(raw);
    Writeln(raw.ReadBytes(256));
    Close(raw);
  end;
end.
