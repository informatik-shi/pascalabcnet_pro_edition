// Shared compatibility corpus: run with ABC Native and PascalABC.NET.
uses PABCSystem;
begin
  Writeln(Abs(-7), ' ', Sqr(50000), ' ', Sign(-2.5));
  Writeln(Min(8, -3, 5, 1), ' ', Max(8, -3, 5, 1));
  Writeln(Round(2.5), ' ', Round(3.5), ' ', Round(-2.5), ' ', Round(-3.5));
  Writeln(Trunc(-2.8), ' ', Floor(-2.8), ' ', Ceil(-2.8));
  Writeln(Sqrt(81) = 9, ' ', Power(2, -3) = 0.125, ' ', ISqrt(9223372036854775807) = 3037000499);
  Writeln(Abs(Sin(Pi / 2) - 1) < 1e-12, ' ', Abs(Ln(E) - 1) < 1e-12);
  Writeln(Abs(DegToRad(180) - Pi) < 1e-12, ' ', Abs(RadToDeg(Pi) - 180) < 1e-12);
  Writeln(Length('Привет'), ' ', Length('🌍'));
  Writeln(Copy('abcdef', 0, 3), ' ', Copy('abcdef', 4, 100));
  Writeln(Pos('abc', 'abcabc'), ' ', PosEx('abc', 'abcabc', 2), ' ', LastPos('abc', 'abcabc', 5));
  Writeln(Pos('', 'abc'), ' ', Pos('x', 'abc'), ' ', LastPos('aa', 'aaaa'));
  Writeln(Concat('Привет, ', 'мир'));
  var n := 10;
  Inc(n);
  Dec(n, 3);
  Writeln(n);
  Writeln(Min(9007199254740993, 9007199254740992, 9007199254740994), ' ', Max(9007199254740993, 9007199254740992));
  Writeln(Sinh(0) = 0, ' ', Cosh(0) = 1, ' ', Tanh(0) = 0, ' ', Abs(Cos(Pi) + 1) < 1e-12, ' ', Abs(Tan(Pi/4) - 1) < 1e-12);
  Writeln(Abs(ArcSin(1) - Pi/2) < 1e-12, ' ', Abs(ArcCos(-1) - Pi) < 1e-12, ' ', Abs(ArcTan(1) - Pi/4) < 1e-12);
  Writeln(Abs(Exp(1) - E) < 1e-12, ' ', Log(1) = 0, ' ', Abs(Log2(8) - 3) < 1e-12, ' ', Abs(Log10(100) - 2) < 1e-12, ' ', Abs(LogN(2, 8) - 3) < 1e-12);
  Writeln(Abs(Cbrt(-27) + 3) < 1e-12, ' ', Int(-2.75) = -2, ' ', Frac(-2.75) = -0.75, ' ', Sign(0) = 0);
  Writeln(Round(1.25,1) = 1.2, ' ', Round(1.35,1) = 1.4, ' ', Round(-1.25,1) = -1.2);
  Writeln(MaxInt = 2147483647, ' ', MaxInt64 = 9223372036854775807, ' ', MaxReal = MaxDouble, ' ', MinReal = MinDouble, ' ', MinReal > 0);
  Writeln(Concat(Copy('🌍',1,1), Copy('🌍',2,1)));
  Writeln(LastPos('a',''), ' ', LastPos('a','',0));
  var large: int64 := MaxInt;
  Inc(large);
  Dec(large, -1);
  Writeln(large);
end.
