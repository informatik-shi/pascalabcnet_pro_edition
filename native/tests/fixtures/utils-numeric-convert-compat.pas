program UtilsNumericConvertSource;
uses Utils;
begin
  Writeln(EvalStr('CONVERT(-128,System.SByte)'), ' ', EvalInt('CONVERT(2.5,System.Byte)'), ' ', EvalStr('CONVERT(''65535'',System.UInt16)'));
  Writeln(EvalStr('CONVERT(''4294967295'',System.UInt32)'), ' ', EvalStr('CONVERT(''18446744073709551615'',System.UInt64)'), ' ', EvalStr('CONVERT(-0.5,System.UInt64)'));
  Writeln(EvalStr('CONVERT(1e-50,System.Single)'), ' ', EvalStr('CONVERT(1e40,System.Single)'), ' ', EvalStr('CONVERT(''1.2e2'',System.Single)'));
  Writeln(EvalStr('CONVERT(CONVERT(0.1,System.Single),System.Decimal)'), ' ', EvalStr('CONVERT(3,System.Single)/CONVERT(2,System.Single)'));
  Writeln(EvalStr('CONVERT(200,System.Byte)+CONVERT(-100,System.SByte)'), ' ', EvalStr('CONVERT(65535,System.UInt16)+CONVERT(1,System.Int16)'));
  Writeln(EvalStr('CONVERT(4294967295,System.UInt32)+1'), ' ', EvalStr('CONVERT(''18446744073709551615'',System.UInt64)%CONVERT(2,System.UInt64)'));
  Writeln(EvalStr('-CONVERT(2,System.Byte)'), ' ', EvalBool('-CONVERT(2,System.UInt32) IS NULL'), ' ', EvalBool('-CONVERT(2,System.SByte) IS NULL'));
  Writeln(EvalBool('CONVERT(1,System.Byte) IN (1.5,2.5)'), ' ', EvalBool('CONVERT(2,System.Byte) IN (1.5,2.5)'), ' ', EvalBool('CONVERT(1,System.UInt64) IN (1.1,2.2)'));
  Writeln(EvalStr('SUBSTRING(''abc'',CONVERT(2,System.Byte),1)'), ' ', EvalStr('SUBSTRING(''abc'',CONVERT(2,System.UInt32),1)'));
  Writeln(EvalBool('TRUE=1'), ' ', EvalBool('TRUE=''true'''), ' ', EvalStr('CONVERT(3,System.Byte)/CONVERT(2,System.Byte)'))
end.
