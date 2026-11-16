program UtilsQualifiedTypesSource;
uses Utils, System;
begin
  Writeln(EvalInt('CONVERT(12,''System.Int32, MSCORLIB'')'));
  Writeln(EvalInt('CONVERT(3.5,'' System.Int32, mscorlib.dll '')'));
  Writeln(EvalInt('CONVERT(12,''System.Int32, mscorlib, Version=99.0.0.0'')'));
  Writeln(EvalInt('CONVERT(12,''System.Int32, mscorlib, imaginary=invalid'')'));
  Writeln(EvalStr('CONVERT(2.25,''System.Decimal, mscorlib, Culture=ru-RU'')'));
  var typeName:=typeof(Guid).AssemblyQualifiedName;
  var boxed:=Eval('CONVERT(''000102030405060708090a0b0c0d0e0f'','''+typeName+''')');
  Writeln(boxed.GetType.FullName,' ',boxed.ToString);
  Writeln(EvalBool('CONVERT(NULL,''System.Byte[], mscorlib'') IS NULL'));
  Writeln(EvalBool('CONVERT(NULL,''System.Int32[][,,,], mscorlib'') IS NULL'));
  Writeln(EvalBool('CONVERT(NULL,''System.String[, ]'') IS NULL'));
  Writeln(EvalBool('CONVERT(NULL,''System.Int32[*]'') IS NULL'));
  Writeln(EvalBool('CONVERT(NULL,''System.Int32&'') IS NULL'));
  Writeln(EvalBool('CONVERT(NULL,''System.Int32**, mscorlib'') IS NULL'));
  Writeln(EvalBool('CONVERT(NULL,''System.IntPtr'') IS NULL'));
  Writeln(EvalBool('CONVERT(NULL,''System.Void[]'') IS NULL'));
  var n:=Eval('CONVERT(NULL,''System.Exception, mscorlib'')');
  Writeln(n=object(DBNull.Value));
end.
