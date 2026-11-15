begin
  var a:=Decimal.Parse('1.00');
  var b:=Decimal.Parse('2.0');
  Writeln(a/b,' ',Decimal.GetBits(a/b));
  Writeln(Decimal.One/3,' ',Decimal.GetBits(Decimal.One/3));
  Writeln(Decimal.Divide(Decimal.Parse('10.000'),4));
  Writeln(Decimal.One/8,' ',Decimal.GetBits(Decimal.One/8));
  Writeln(Decimal.MaxValue/10,' ',Decimal.MaxValue/3);
  Writeln(Decimal.GetBits(Decimal.Parse('0.000')/b));
  Writeln(Decimal.GetBits(Decimal.Parse('-0.0')/b));
  Writeln(Decimal.GetBits(Decimal.Parse('0.0000000000000000000000000001')/2));
  Writeln(Decimal.GetBits(Decimal.Remainder(Decimal.Parse('1.2'),Decimal.Parse('3.400'))));
  Writeln(Decimal.Remainder(Decimal.Parse('-9.4'),Decimal.Parse('2.0')),' ',Decimal.Remainder(Decimal.Parse('9.4'),Decimal.Parse('-2.0')));
  Writeln(Decimal.GetBits(Decimal.Remainder(Decimal.Parse('-4.2'),Decimal.Parse('2.10'))));
  Writeln(Decimal.Round(Decimal.Parse('1.345'),2),' ',Decimal.Round(Decimal.Parse('1.355'),2));
  Writeln(Decimal.Round(Decimal.Parse('-2.5')),' ',Decimal.Round(Decimal.Parse('-3.5')));
  Writeln(Decimal.Round(Decimal.Parse('1.20'),5),' ',Decimal.GetBits(Decimal.Truncate(Decimal.Parse('1.999'))));
  Writeln(Decimal.Floor(Decimal.Parse('-1.01')),' ',Decimal.Ceiling(Decimal.Parse('-1.01')),
    ' ',Decimal.Floor(Decimal.Parse('1.01')),' ',Decimal.Ceiling(Decimal.Parse('1.01')));
  Writeln(Decimal.GetBits(Decimal.Ceiling(Decimal.Parse('-0.1'))),' ',Decimal.GetBits(Decimal.Round(Decimal.Parse('-0.4'))));
  Writeln(Decimal.Parse('9.4') mod Decimal.Parse('2.0'));
end.
