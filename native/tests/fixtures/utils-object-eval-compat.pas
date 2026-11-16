program UtilsObjectEvalSource;
uses Utils, System;
procedure Show(s:string);
begin
  var v:=Utils.Eval(s);
  Writeln(v.GetType.FullName,' [',v.ToString,']')
end;
begin
  Show('42');
  Show('2147483648');
  Show('0.1+0.2');
  Show('1e1');
  Show('CONVERT(7,System.Single)');
  Show('CONVERT(255,System.Byte)');
  Show('CONVERT(''18446744073709551615'',System.UInt64)');
  Show('CONVERT(65,System.Char)');
  Show('''Привет🌍''');
  Show('TRUE');
  Show('NULL');
  var a:=Utils.Eval('NULL');
  var b:=Utils.Eval('');
  Writeln(a=b,' ',a=object(DBNull.Value),' ',a=nil);
  Writeln(a.GetType.Name, ' ', a.GetType.Namespace);
  Writeln(System.DBNull.Value.ToString.Length,' ',a.GetType=typeof(System.DBNull));
  Writeln(a.GetType.AssemblyQualifiedName);
  var f:function(s:string):object:=Utils.Eval;
  Writeln(integer(f('123')));
end.
