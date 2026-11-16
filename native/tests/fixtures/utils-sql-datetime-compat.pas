program UtilsSqlDateTimeCompatibility;
uses Utils, System, System.Data.SqlTypes;

type TSqlDateTime = System.Data.SqlTypes.SqlDateTime;

begin
  var exact:=SqlDateTime.Parse('2024-02-29T12:30:20.025');
  Writeln(exact.Value.ToString('yyyy-MM-dd HH:mm:ss.fffffff'),' ',exact.DayTicks,' ',exact.TimeTicks);
  Writeln(exact.Value.Kind,' ',exact.ToString);
  Writeln(SqlDateTime.Parse('Jan 30, 2024').Value.ToString('yyyy-MM-dd'));
  Writeln(SqlDateTime.Parse('1/2/30 3:04:05 PM').Value.ToString('yyyy-MM-dd HH:mm:ss'));
  Writeln(DateTime.Parse('1/2/49').Year,' ',DateTime.Parse('1/2/50').Year);
  Writeln(SqlDateTime.Parse('1/2/49').Value.Year,' ',SqlDateTime.Parse('1/2/50').Value.Year);
  Writeln(SqlDateTime.Parse('07:15:00').DayTicks=(new SqlDateTime(DateTime.Today)).DayTicks);
  Writeln((new SqlDateTime(-1,25919999)).Value.ToString('yyyy-MM-dd HH:mm:ss.fff'));
  Writeln((new SqlDateTime(0,1)).Value.ToString('yyyy-MM-dd HH:mm:ss.fff'));
  Writeln((new SqlDateTime(0,2)).Value.ToString('yyyy-MM-dd HH:mm:ss.fff'));
  Writeln(SqlDateTime.Parse('1900-01-01T23:59:59.999').Value.ToString('yyyy-MM-dd HH:mm:ss.fff'));
  Writeln(SqlDateTime.MinValue.Value.ToString('yyyy-MM-dd HH:mm:ss.fff'),' ',SqlDateTime.MinValue.DayTicks);
  Writeln(SqlDateTime.MaxValue.Value.ToString('yyyy-MM-dd HH:mm:ss.fff'));
  Writeln((new SqlDateTime(DateTime.MaxValue)).CompareTo(SqlDateTime.MaxValue));
  var empty: TSqlDateTime;
  Writeln(SqlDateTime.Parse('Null').IsNull,' ',empty.ToString);
  Writeln(SqlDateTime.Add(exact,TimeSpan.FromSeconds(1)).Value.ToString('HH:mm:ss.fff'));
  Writeln(SqlDateTime.Subtract(exact,TimeSpan.FromSeconds(1)).Value.ToString('HH:mm:ss.fff'));
  var raw:=Eval('CONVERT(#2024-02-29 12:30:20.025#,System.Data.SqlTypes.SqlDateTime)');
  var typed:=TSqlDateTime(raw);
  Writeln(raw.GetType.FullName,' ',typed.TimeTicks);
  Writeln(EvalStr('CONVERT(#2024-02-29 12:30:20.025#,System.Data.SqlTypes.SqlDateTime)=#2024-02-29 12:30:20.026#'));
  Writeln(EvalStr('CONVERT(''P1D'',System.TimeSpan)-CONVERT(#2024-02-29#,System.Data.SqlTypes.SqlDateTime)'));
end.
