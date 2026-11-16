program UtilsSqlDecimalCompatibility;
uses Utils, System.Data.SqlTypes;

type TSqlDecimal = System.Data.SqlTypes.SqlDecimal;

begin
  var big:=SqlDecimal.Parse('99999999999999999999999999999999999999');
  var bits:=big.Data;
  Writeln(big.ToString,' ',big.Precision,' ',big.Scale);
  Writeln(bits[0],' ',bits[1],' ',bits[2],' ',bits[3]);
  var scaled:=new SqlDecimal(Decimal.Parse('-12.3400'));
  Writeln(scaled.ToString,' ',scaled.Precision,' ',scaled.Scale,' ',scaled.IsPositive);
  Writeln(scaled.Value.ToString);
  Writeln(SqlDecimal.Parse('.00012').ToString);
  Writeln(SqlDecimal.Parse('0.').ToString);
  var empty: SqlDecimal;
  Writeln(SqlDecimal.Parse('Null').IsNull,' ',empty.ToString,' ',TSqlDecimal.Null.IsNull);
  Writeln(SqlDecimal.MinValue.ToString);
  Writeln(SqlDecimal.MaxValue.ToString);
  Writeln(SqlDecimal.Parse('1.200').CompareTo(SqlDecimal.Parse('1.2')));
  Writeln(SqlDecimal.Add(SqlDecimal.Parse('1.30'),SqlDecimal.Parse('2.700')).ToString);
  Writeln(SqlDecimal.Subtract(SqlDecimal.Parse('1.30'),SqlDecimal.Parse('2.700')).ToString);
  Writeln(SqlDecimal.Multiply(SqlDecimal.Parse('1.30'),SqlDecimal.Parse('2.700')).ToString);
  Writeln(SqlDecimal.Divide(SqlDecimal.Parse('1'),SqlDecimal.Parse('3')).ToString);
  Writeln(SqlDecimal.Divide(SqlDecimal.Parse('1'),SqlDecimal.Parse('3')).Scale);
  var nativeBox:=Eval('CONVERT(CONVERT(''12345678901234567890'',System.UInt64),System.Data.SqlTypes.SqlDecimal)');
  var unboxed:=TSqlDecimal(nativeBox);
  Writeln(nativeBox.GetType.FullName,' ',unboxed.ToString);
  Writeln(EvalStr('CONVERT(5,System.Data.SqlTypes.SqlInt32)+2.25'));
  Writeln(EvalStr('CONVERT(5,System.Data.SqlTypes.SqlInt32)/2.25'));
  Writeln(EvalStr('CONVERT(7,System.Data.SqlTypes.SqlDecimal) IN (5,7)'));
  Writeln(EvalStr('CONVERT(CONVERT(0.125,System.Data.SqlTypes.SqlDecimal),System.Data.SqlTypes.SqlSingle)'));
end.
