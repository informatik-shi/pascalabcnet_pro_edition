begin
  var d: decimal;
  Writeln(d,' ',d.GetType.FullName);
  var a:=Decimal.Parse('1.2300');
  var b:=Decimal.Parse(' 2,345.6 ');
  Writeln(a,' ',b,' ',a+7,' ',b-a,' ',a*Decimal.Parse('2.0'));
  Writeln(Decimal.GetBits(a));
  Writeln(Decimal.MaxValue,' ',Decimal.MinValue);
  var raw:=new Decimal(-1,-1,-1,False,byte(28));
  Writeln(raw,' ',Decimal.GetBits(raw));
  var exact:=new Decimal(Arr(123,0,0,196608));
  Writeln(exact,' ',Decimal.GetBits(exact),' ',exact=Decimal.Parse('0.1230'));
  Writeln(-exact,' ',Decimal.ToInt32(Decimal.Parse('-123.99')),' ',Decimal.ToInt64(Decimal.Parse('9007199254740993')));
  Writeln(Decimal(123456789.12345679),' ',Decimal(single(1.23456789)),' ',Decimal(1e-28));
  Writeln(Decimal.Parse('0.00000000000000000000000000005'),' ',Decimal.Parse('0.00000000000000000000000000015'));
  Writeln(Decimal.MaxValue+Decimal.Parse('0.4'));
  Writeln(Decimal.Compare(Decimal.One,Decimal.Parse('1.000')),' ',Decimal.MinusOne<Decimal.Zero);
  Writeln(Decimal.GetBits(new Decimal(0,0,0,True,byte(2))));
  Writeln(Decimal.GetBits(Decimal(5e-29)),' ',Decimal.GetBits(Decimal(single(5e-29))));
  d:=Decimal.Parse('1.25');
  Writeln(d,' ',d.ToString);
  SetDecimalSeparator(',');
  Writeln(d,' ',d.ToString);
  NumberFormatInfo.CurrentInfo.NumberDecimalSeparator:='::';
  Writeln(d,' ',d.ToString,' ',Decimal.TryParse('2::50',d),' ',d,' ',d.ToString);
  Writeln(Decimal.TryParse('?',d),' ',d);
end.
