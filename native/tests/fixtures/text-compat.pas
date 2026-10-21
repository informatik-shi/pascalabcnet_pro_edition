// Exact-output compatibility corpus for the next PABCSystem slice.
begin
  Writeln('[', Trim('  abc  '), '] [', TrimLeft('  abc  '), '] [', TrimRight('  abc  '), ']');
  Writeln(Length(Trim('   ')), ' ', Length(Trim(' ')));
  Writeln(LeftStr('Привет', 3), ' ', RightStr('Привет', 3));
  Writeln(LeftStr('abc', -1) = '', ' ', RightStr('abc', 100) = 'abc');
  Writeln(ReverseString('abcdef'), ' ', ReverseString('abcdef', 2, 3));
  Writeln(ReverseString('', 1, 0) = '', ' ', ReverseString('abc', 4, 0) = 'abc');
  Writeln(CompareStr('abc', 'abd') < 0, ' ', CompareStr('abc', 'abc') = 0, ' ', CompareStr('abcd', 'abc') > 0);
  Writeln(IntToStr(-2147483648), ' ', IntToStr(-9223372036854775808));
  Writeln(StrToInt(' +2147483647 '), ' ', StrToInt('-2147483648'));
  Writeln(StrToInt(' 42 '));
  Writeln(StrToInt64(' +9223372036854775807 '), ' ', StrToInt64('-9223372036854775808'));
  Writeln(StrToReal('1,234.5', '.') = 1234.5, ' ', StrToFloat(' +1.25e2 ') = 125);
  Writeln(StrToReal('1,25', ',') = 1.25, ' ', StrToFloat('1::25', '::') = 1.25);
  Writeln(StrToReal('.5') = 0.5, ' ', StrToReal('5.') = 5, ' ', StrToReal('1e-999') = 0);
  Writeln(FloatToStr(1.25), ' ', FloatToStr(-0.5), ' ', FloatToStr(1000000.0));
  Writeln(FloatToStr(0.0001), ' ', FloatToStr(0.00001), ' ', FloatToStr(1e16), ' ', FloatToStr(1e17));
  Writeln(StrToInt(IntToStr(MaxInt)) = MaxInt, ' ', StrToInt64(IntToStr(MaxInt64)) = MaxInt64);
  Writeln(StrToReal(FloatToStr(1.2345678901234567)) = 1.2345678901234567);
end.
