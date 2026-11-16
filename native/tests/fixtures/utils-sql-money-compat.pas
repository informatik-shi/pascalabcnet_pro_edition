program UtilsSqlMoneyCompatibility;
uses Utils, System.Data.SqlTypes;

type TSqlMoney = System.Data.SqlTypes.SqlMoney;

begin
  var initial:=new SqlMoney(Decimal.Parse('1.23005'));
  Writeln(initial.ToString,' ',initial.Value.ToString);
  Writeln(SqlMoney.Parse('($1,234.5)').ToString);
  Writeln(SqlMoney.Parse('¤1.23').ToString);
  Writeln(SqlMoney.Parse('0.00005').ToString,' ',SqlMoney.Parse('-0.00005').ToString);
  Writeln(SqlMoney.Parse('1.50').ToInt64,' ',SqlMoney.Parse('-1.50').ToInt64);
  Writeln(SqlMoney.Parse('-1.90').ToInt32);
  Writeln(SqlMoney.MaxValue.ToString);
  Writeln(SqlMoney.MinValue.ToString);
  Writeln(SqlMoney.Zero.ToString);
  Writeln(SqlMoney.Parse('Null').IsNull,' ',SqlMoney.Null.ToString);
  var generated:=SqlMoney.Divide(SqlMoney.Parse('7'),SqlMoney.Parse('3'));
  Writeln(generated.ToString,' ',generated.Value.ToString);
  Writeln(SqlMoney.Multiply(SqlMoney.Parse('-0.0001'),SqlMoney.Parse('0.5000')).ToString);
  Writeln(SqlMoney.Parse('1.00').CompareTo(SqlMoney.Parse('1.0')));
  var raw:=Eval('CONVERT(12.025,System.Data.SqlTypes.SqlMoney)');
  var typed:=TSqlMoney(raw);
  Writeln(raw.GetType.FullName,' ',typed.ToString);
  Writeln(EvalStr('CONVERT(12.025,System.Data.SqlTypes.SqlMoney)+7'));
  Writeln(EvalStr('CONVERT(12.025,System.Data.SqlTypes.SqlMoney)+0.005'));
  Writeln(EvalStr('CONVERT(12.025,System.Data.SqlTypes.SqlMoney)/7'));
  Writeln(EvalStr('CONVERT(CONVERT(12.025,System.Data.SqlTypes.SqlMoney),System.Data.SqlTypes.SqlDecimal)'));
end.
