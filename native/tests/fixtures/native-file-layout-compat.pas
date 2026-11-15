uses System;
type TData = record
  amount: Decimal;
  interval: TimeSpan;
  number: Complex;
end;

begin
  var value, restored: TData;
  value.amount:=new Decimal($12345678,$1234567,$1234,True,byte(6));
  value.interval:=TimeSpan.FromTicks(-1234567890123);
  value.number:=Cplx(-1.25,3.5);

  var f: file of TData;
  Assign(f,'native-record-layout.dat');
  Rewrite(f);
  Write(f,value);
  Close(f);
  Reset(f);
  Writeln(FileSize(f));
  Read(f,restored);
  Writeln(Decimal.GetBits(restored.amount));
  Writeln(restored.interval.Ticks,' ',restored.number);
  Writeln(FilePos(f),' ',Eof(f));
  Close(f);
  var raw: file;
  Assign(raw,'native-record-layout.dat');
  Reset(raw);
  Writeln(raw.ReadBytes(64));
  Close(raw);
end.
