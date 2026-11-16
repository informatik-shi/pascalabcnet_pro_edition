program UtilsDateTimeOffsetSource;
uses Utils, System;
function ValueOf(s:string):DateTimeOffset := DateTimeOffset(Eval('CONVERT('''+s+''',System.DateTimeOffset)'));
procedure Show(s:string);
begin
  var value:=ValueOf(s);
  Writeln(value.GetType.FullName,'|',value.ToString('O'),'|',value.ToString,'|',
    value.UtcDateTime.ToString('O'),'|',value.Offset.Ticks)
end;
begin
  Show('2024-02-29T07:20:18.1234567+03:00');
  Show('2024-02-29 12:34:56-05:30');
  Show('2024-02-29T12:34:56Z');
  Show('2024-02-29T12:34:56+0300');
  Show('2024-02-29T12:34:56+3:00');
  Show('2/29/2024 1:05 PM +03:30');
  Show('Feb. 29, 2024 7:20 +03:00');
  Show('2024-02-29 +03:00');
  Show('2024-02-29T12:34:56.12345675+03:00');
  Show('2024-02-29T12:34:56.12345665+03:00');
  Show('2024-02-29T23:59:59.99999999Z');
  Show('2024-02-29T12:00:00-0:30');
  Show('2024-02-29T12:00:00+030');
  Show('2024-02-29T00:00:00+14:00');
  Show('2024-02-29T00:00:00-14:00');
  var a:System.DateTimeOffset:=System.DateTimeOffset.Parse('2024-02-29T12:34:56+03:00');
  var b:=DateTimeOffset.Parse('2024-02-29T09:34:56Z');
  Writeln(a=b,' ',a.Equals(b),' ',a.EqualsExact(b),' ',a.CompareTo(b),' ',a.GetHashCode=b.GetHashCode);
  Writeln(a.DateTime.Kind,' ',a.UtcDateTime.Kind,' ',a.Ticks-a.UtcTicks=a.Offset.Ticks);
  Writeln(a.ToString('G'),'|',a.ToString('yyyy-MM-dd HH:mm zzz'),'|',a.ToString('u'));
  var shifted:=a.ToOffset(TimeSpan.FromMinutes(-90));
  Writeln(shifted.ToString('O'),' ',shifted=a);
  var fromTicks:=new DateTimeOffset(a.Ticks,a.Offset);
  var fromDate:=new DateTimeOffset(a.DateTime,a.Offset);
  Writeln(fromTicks.EqualsExact(a),' ',fromDate.EqualsExact(a),' ',DateTimeOffset.MinValue.Ticks,' ',DateTimeOffset.MaxValue.DateTime.Year);
  var implicitOffset:=ValueOf('2024-02-29 10:30:00');
  Writeln(implicitOffset.UtcTicks=DateTime.Parse('2024-02-29 10:30:00').ToUniversalTime.Ticks);
  var index:=new Dictionary<DateTimeOffset,integer>;
  index[a]:=23;
  Writeln(index.ContainsKey(b),' ',index[b]);
  Writeln(EvalBool('CONVERT(''2024-02-29T12:34:56+03:00'',System.DateTimeOffset) IN (CONVERT(''2024-02-29T09:34:56Z'',System.DateTimeOffset))'));
  Writeln(EvalStr('CONVERT(''2024-02-29T12:34:56+03:00'',System.DateTimeOffset)'));
end.
