program UtilsTimeSpanExpressionSource;
uses Utils, System;
procedure Show(s:string);
begin
  var value:=Utils.Eval(s);
  Writeln(value.GetType.FullName,'|',TimeSpan(value).Ticks,'|',Utils.EvalStr(s),'|',
    Utils.EvalStr('CONVERT('+s+',System.String)'))
end;
begin
  Show('CONVERT(''P3DT1H2M3.123456789S'',System.TimeSpan)');
  Show('CONVERT(''-PT0.1S'',''System.TimeSpan'')');
  Show('CONVERT(''  PT.S  '',System.TimeSpan)');
  Show('CONVERT(''PT1.S'',System.TimeSpan)');
  Show('CONVERT(''P1Y2M'',System.TimeSpan)');
  Show('CONVERT(''P13M'',System.TimeSpan)');
  Show('CONVERT(''PT99H'',System.TimeSpan)');
  Show('CONVERT(1,System.TimeSpan)');
  Show('CONVERT(CONVERT(-1,System.Int64),System.TimeSpan)');
  Show('CONVERT(''P10675199DT2H48M5.4775807S'',System.TimeSpan)');
  Show('CONVERT(''-P10675199DT2H48M5.4775808S'',System.TimeSpan)');
  Writeln(EvalStr('CONVERT(''PT1H'',System.TimeSpan)+''!'''));
  var value:=Eval('CONVERT(NULL,System.DBNull)');
  Writeln(value=object(DBNull.Value),' ',EvalBool('CONVERT(NULL,System.TimeSpan) IS NULL'));
end.
