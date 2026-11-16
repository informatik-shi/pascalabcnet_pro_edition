program UtilsSqlOperationsCompatibility;
uses Utils;

procedure Show(expression: string);
begin
  var value:=Eval(expression);
  Writeln(value.GetType.FullName,'|',value.ToString,'|',EvalStr(expression));
end;

begin
  Show('(CONVERT(CONVERT(9,System.Byte),System.Data.SqlTypes.SqlByte))/(CONVERT(CONVERT(-13,System.Int16),System.Data.SqlTypes.SqlInt16))');
  Show('(CONVERT(CONVERT(9,System.Byte),System.Data.SqlTypes.SqlByte))/(CONVERT(CONVERT(9007199254740993,System.Int64),System.Data.SqlTypes.SqlInt64))');
  Show('(CONVERT(CONVERT(9,System.Byte),System.Data.SqlTypes.SqlByte))+(CONVERT(2,System.Byte))');
  Show('(CONVERT(CONVERT(9,System.Byte),System.Data.SqlTypes.SqlByte))%(CONVERT(2,System.Byte))');
  Show('(CONVERT(CONVERT(-13,System.Int16),System.Data.SqlTypes.SqlInt16))/(CONVERT(15,System.UInt16))');
  Show('(CONVERT(CONVERT(-13,System.Int16),System.Data.SqlTypes.SqlInt16))<(2.25)');
  Show('(CONVERT(CONVERT(9007199254740993,System.Int64),System.Data.SqlTypes.SqlInt64))+(CONVERT(CONVERT(-13,System.Int16),System.Data.SqlTypes.SqlInt16))');
  Show('(CONVERT(CONVERT(9007199254740993,System.Int64),System.Data.SqlTypes.SqlInt64))/(CONVERT(CONVERT(-13,System.Int16),System.Data.SqlTypes.SqlInt16))');
  Show('(CONVERT(CONVERT(9007199254740993,System.Int64),System.Data.SqlTypes.SqlInt64))/(-2)');
  Show('(CONVERT(CONVERT(12.5e0,System.Single),System.Data.SqlTypes.SqlSingle))/(CONVERT(CONVERT(9,System.Byte),System.Data.SqlTypes.SqlByte))');
  Show('(CONVERT(CONVERT(12.5e0,System.Single),System.Data.SqlTypes.SqlSingle))/(CONVERT(CONVERT(-13,System.Int16),System.Data.SqlTypes.SqlInt16))');
  Show('(CONVERT(CONVERT(12.5e0,System.Single),System.Data.SqlTypes.SqlSingle))/(CONVERT(CONVERT(9007199254740993,System.Int64),System.Data.SqlTypes.SqlInt64))');
  Show('(CONVERT(CONVERT(12.5e0,System.Single),System.Data.SqlTypes.SqlSingle))/(CONVERT(30,System.UInt32))');
  Show('(CONVERT(-2.5e0,System.Data.SqlTypes.SqlDouble))/(CONVERT(CONVERT(9,System.Byte),System.Data.SqlTypes.SqlByte))');
  Show('(CONVERT(-2.5e0,System.Data.SqlTypes.SqlDouble))+(CONVERT(CONVERT(9007199254740993,System.Int64),System.Data.SqlTypes.SqlInt64))');
  Show('-(CONVERT(CONVERT(9,System.Byte),System.Data.SqlTypes.SqlByte))');
  Show('-(CONVERT(24,System.Data.SqlTypes.SqlInt32))');
  Show('(CONVERT(true,System.Data.SqlTypes.SqlBoolean)) AND (CONVERT(true,System.Data.SqlTypes.SqlBoolean))');
  Show('(CONVERT(true,System.Data.SqlTypes.SqlBoolean)) AND (false)');
  Show('(CONVERT(false,System.Data.SqlTypes.SqlBoolean)) AND (NULL)');
  Show('(false) OR (CONVERT(true,System.Data.SqlTypes.SqlBoolean))');
  Show('(NULL) OR (CONVERT(true,System.Data.SqlTypes.SqlBoolean))');
  Show('(CONVERT(''example'',System.Data.SqlTypes.SqlString)) + (''Example'')');
  Show('(CONVERT(''example'',System.Data.SqlTypes.SqlString)) LIKE (''Example'')');
  Show('(CONVERT(''example'',System.Data.SqlTypes.SqlString)) IN (''ex%'')');
  Show('CONVERT(1,System.Data.SqlTypes.SqlInt32)+CONVERT(0,System.Data.SqlTypes.SqlInt32)');
  Show('CONVERT(1,System.Data.SqlTypes.SqlInt32)/CONVERT(2,System.Data.SqlTypes.SqlInt32)');
  Show('CONVERT(1,System.Data.SqlTypes.SqlInt32)%CONVERT(2,System.Data.SqlTypes.SqlInt32)');
  Show('NOT CONVERT(TRUE,System.Data.SqlTypes.SqlBoolean)');
  Show('CONVERT(5,System.Data.SqlTypes.SqlInt32) IN (4,NULL,5)');
  Show('CONVERT(CONVERT(0,System.Byte),System.Data.SqlTypes.SqlByte)-CONVERT(CONVERT(0,System.Byte),System.Data.SqlTypes.SqlByte)');
  Show('''n='' + CONVERT(42,System.Data.SqlTypes.SqlInt32)');
  Show('CONVERT(''abc'',System.Data.SqlTypes.SqlString) LIKE ''a%''');
  Show('ABS(CONVERT(5,System.Data.SqlTypes.SqlInt32))');
end.
