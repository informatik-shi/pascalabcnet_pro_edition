program UtilsExpressionFunctionsSource;
uses Utils;
begin
  Writeln(EvalInt('LEN(''🌍'')'), ' ', EvalInt('LEN(TRIM(''  yes  ''))'), ' ', EvalBool('LEN(NULL) IS NULL'));
  Writeln('[', EvalStr('SUBSTRING(''abc'',2,9)'), '] [', EvalStr('SUBSTRING(''abc'',4,2)'), ']');
  Writeln(EvalBool('SUBSTRING(''abc'',5,2) IS NULL'), ' ', EvalBool('SUBSTRING(''abc'',5,0) IS NULL'));
  Writeln(Ord(EvalStr('SUBSTRING(''😀'',1,1)')[1]), ' ', Ord(EvalStr('SUBSTRING(''😀'',2,1)')[1]));
  Writeln(EvalInt('IIF(TRUE,7,8)'), ' ', EvalInt('IIF(NULL,7,8)'), ' ', EvalInt('IIF(''true'',7,8)'));
  Writeln(EvalInt('ISNULL(NULL,42)'), ' ', EvalInt('ISNULL(0,42)'), ' ', EvalStr('ISNULL(''x'',''y'')'));
  Writeln(EvalBool('ABS(-3) IS NULL'), ' ', EvalBool('ABS(''x'') IS NULL'), ' ', EvalInt('NULL OR 1'));
  Writeln(EvalBool('FALSE AND 1'), ' ', EvalBool('TRUE OR 1'), ' ', EvalBool('0/0 = 0/0'));
  Writeln(EvalStr('IIF(NULL, TRIM('' wrong ''), ISNULL(NULL, SUBSTRING(''good'',2,3)))'))
end.
