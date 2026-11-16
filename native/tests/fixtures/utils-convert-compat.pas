program UtilsConvertSource;
uses Utils;
begin
  Writeln(EvalInt('CONVERT(''123'',System.Int32)'), ' ', EvalInt('CONVERT(2.5,''System.Int32'')'), ' ', EvalInt('CONVERT(-3.5,''System.Int32'')'));
  Writeln(EvalStr('CONVERT(''9223372036854775807'',System.Int64)'), ' ', EvalStr('CONVERT(-1,''System.Object'')'));
  Writeln(EvalReal('CONVERT(''1.25'',''System.Double'')'), ' ', EvalStr('CONVERT(1e-3,''System.Decimal'')'), ' ', EvalStr('CONVERT(true,''System.Decimal'')'));
  Writeln(EvalBool('CONVERT(''  true  '',''System.Boolean'')'), ' ', EvalBool('CONVERT(''1'',''System.Boolean'')'), ' ', EvalBool('CONVERT(''0'',''System.Boolean'')'));
  Writeln(EvalBool('CONVERT(-3,''System.Boolean'')'), ' ', EvalBool('CONVERT(0/0,''System.Boolean'')'), ' ', EvalBool('CONVERT(NULL,System.String) IS NULL'));
  Writeln(EvalInt('CONVERT(''A'',''System.Char'')'), ' ', EvalStr('CONVERT(66,''System.Char'')'), ' ', EvalStr('CONVERT(''A'',''System.Char'')+CONVERT(''B'',''System.Char'')'));
  Writeln(EvalBool('CONVERT(''A'',''System.Char'')=''a'''), ' ', EvalStr('CONVERT(LEN(CONVERT(12,System.String)),System.String)'));
  Writeln(EvalStr('IIF(CONVERT(''0'',System.Boolean),''left'',CONVERT(2.5,System.String))'))
end.
