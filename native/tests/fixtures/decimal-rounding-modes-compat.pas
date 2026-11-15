uses System;
begin
  Writeln(Decimal.Round(Decimal.Parse('2.5'),MidpointRounding.ToEven),' ',
    Decimal.Round(Decimal.Parse('2.5'),MidpointRounding.AwayFromZero));
  Writeln(Decimal.Round(Decimal.Parse('-2.5'),MidpointRounding.ToEven),' ',
    Decimal.Round(Decimal.Parse('-2.5'),MidpointRounding.AwayFromZero));
  Writeln(Decimal.Round(Decimal.Parse('1.345'),2,MidpointRounding.AwayFromZero));
  Writeln(Decimal.ToByte(Decimal.Parse('255.9')),' ',Decimal.ToSByte(Decimal.Parse('-128.9')),
    ' ',Decimal.ToInt16(Decimal.Parse('-32768.2')),' ',Decimal.ToUInt16(Decimal.Parse('65535.9')));
  Writeln(byte(Decimal.Parse('7.99')),' ',shortint(Decimal.Parse('-7.99')),
    ' ',smallint(Decimal.Parse('31000.999')),' ',word(Decimal.Parse('64000.999')));
  Writeln(char(Decimal(65)),' ',Decimal('A'));
  Writeln(Decimal.Zero.GetHashCode,' ',Decimal.One.GetHashCode,' ',Decimal.MinusOne.GetHashCode);
  Writeln(Decimal.Parse('1.0000').GetHashCode,' ',Decimal.Parse('1.23').GetHashCode,
    ' ',Decimal.Parse('1.230').GetHashCode,' ',Decimal.MaxValue.GetHashCode);
end.
