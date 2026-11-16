program UtilsSqlTypesCompatibility;
uses Utils, System.Data.SqlTypes;

type TQualifiedSql = System.Data.SqlTypes.SqlInt32;

begin
  var none: SqlInt32;
  var boxed:=Eval('CONVERT(42,System.Data.SqlTypes.SqlInt32)');
  var typed:=TQualifiedSql(boxed);
  var fullyQualified:=System.Data.SqlTypes.SqlInt32(boxed);
  var unboxed: Integer:=typed.Value;
  Writeln(boxed.GetType.FullName,' ',unboxed,' ',fullyQualified.Value);
  Writeln(typed.GetType.AssemblyQualifiedName);
  Writeln(none.IsNull,' ',none.ToString,' ',TQualifiedSql.Null.IsNull);
  Writeln(SqlInt32.MinValue.Value,' ',SqlInt32.Zero.Value,' ',SqlInt32.MaxValue.Value);
  Writeln(SqlByte.MinValue.Value,' ',SqlByte.MaxValue.Value,' ',SqlInt64.MaxValue.Value);
  var yes:=new SqlBoolean(True);
  var no:=new SqlBoolean(False);
  Writeln(yes.IsTrue,' ',no.IsFalse,' ',SqlBoolean.Null.IsFalse);
  Writeln(SqlBoolean.True.Value,' ',SqlBoolean.False.Value,' ',(new SqlString(nil)).IsNull);
  Writeln((new SqlString('привет')).Value);
  Writeln(EvalStr('CONVERT(CONVERT(255,System.Byte),System.Data.SqlTypes.SqlByte)'));
  Writeln(EvalStr('CONVERT(CONVERT(-32768,System.Int16),System.Data.SqlTypes.SqlInt16)'));
  Writeln(EvalStr('CONVERT(CONVERT(4294967295,System.UInt32),System.Data.SqlTypes.SqlInt64)'));
  Writeln(EvalStr('CONVERT(CONVERT(5,System.Data.SqlTypes.SqlInt32),System.Data.SqlTypes.SqlDouble)'));
  Writeln(EvalStr('CONVERT(CONVERT(125e-3,System.Single),System.Data.SqlTypes.SqlSingle)'));
  Writeln(EvalStr('CONVERT(False,System.Data.SqlTypes.SqlBoolean)'));
  Writeln(EvalStr('CONVERT('''',System.Data.SqlTypes.SqlString)').Length);
  Writeln(Eval('CONVERT(NULL,System.Data.SqlTypes.SqlInt32)').GetType.FullName);
end.
