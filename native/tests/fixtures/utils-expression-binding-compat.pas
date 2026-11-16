program UtilsExpressionBindingSource;
uses Utils;
begin
  Writeln(Utils.EvalInt('[LEN](''Привет'')'));
  Writeln(Utils.EvalStr('`Trim`(''   PascalABC  '')'));
  Writeln(Utils.EvalStr('[IIF](TRUE,''C++'',''other'')'));
  Writeln(Utils.EvalInt('CONVERT(4,System . Int32)'));
  Writeln(Utils.EvalInt('[Convert](5,''System.Int32'')'));
  Writeln(Utils.EvalBool('[Abs](-2.5) IS NULL'));
end.
