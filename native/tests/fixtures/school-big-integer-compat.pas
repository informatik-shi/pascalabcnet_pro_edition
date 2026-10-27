uses School;
begin
  var n := DecBig('FFFFFFFFFFFFFFFFFFFFFFFF',16);
  Writeln(n);
  Writeln(n.ToBase(16));
  Writeln(n.Digits(16).DigitsToBigInteger(16)=n);
  Writeln(Bin(-256bi),' ',Oct(256bi),' ',Hex(256bi));
  Writeln(BinFormat(256bi,3,':'));
  Writeln(ToBase('18446744073709551616',16),'|',ToBase('bad',16));
  Writeln(School.GCD(Arr(-18bi,30bi,42bi)));
  Writeln(School.LCM(Arr(18446744073709551616bi,3bi)));
  Writeln(MinMax(Arr(-n,0bi,n)));
  Writeln(MinMax(new BigInteger[0]));
end.
