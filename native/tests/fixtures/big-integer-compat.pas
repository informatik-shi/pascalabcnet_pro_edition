begin
  var a: BigInteger := 123456789012345678901234567890bi;
  Writeln(a+10,' ',a*2);
  Writeln((-11bi) div 2,' ',(-11bi) mod 2);
  Writeln(1bi/2,' ',5bi/2bi);
  Writeln((1bi shl 80) shr 76,' ',(-9bi) shr 1);
  Writeln(BigInteger.Parse(' +000123 '),' ',BigInteger.Pow(2,100));
  Writeln(GCD(48bi,18bi),' ',LCM(-4bi,6bi),' ',Sqr(12bi));
  Writeln(ISqrt(a*a));
  var b: BigInteger;
  Writeln(BigInteger.TryParse('17',b),' ',b);
  Writeln(BigInteger.TryParse('bad',b),' ',b);
  Writeln(BigInteger.Zero.IsZero,' ',BigInteger.One.IsOne,' ',BigInteger.MinusOne.Sign);
end.
