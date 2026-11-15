uses System;
begin
  var d:=new DateTime(2024,2,29,13,5,9,120);
  d:=d.AddTicks(3456);
  var utc:=DateTime.SpecifyKind(d,DateTimeKind.Utc);
  Writeln(d.ToString('yyyy-MM-dd HH:mm:ss.fffffff'));
  Writeln(d.ToString('dddd, MMMM d ''at'' h:mm tt'));
  Writeln(d.ToString('MMM dd yy'));
  Writeln(utc.ToString('yyyy-MM-ddTHH:mm:ss.FFFFFFFK'));
  Writeln(DateTime.MinValue.ToString('yyyy-MM-dd HH:mm:ss.FFFFFFF'));
  Writeln(d.ToString('yyyy\/MM\/dd %H'));
  Writeln(utc.ToString('[%d] [%M] [%y] [%K]'));
  Writeln(Format('{0:yyyy-MM-dd} {0:HH:mm}',d));
  var boxed:object:=d;
  d:=DateTime.MinValue;
  Writeln(boxed.GetType.FullName,' ',DateTime(boxed).ToString('s'));
  var duration:object:=TimeSpan.FromMinutes(-1.5);
  Writeln(duration.GetType.FullName,' ',TimeSpan(duration).Ticks,' ',Format('{0:c}',duration));
  Writeln(d.ToString('M'),' ',d.ToString('Y'),' ',utc.ToString('R'));
  var span := new TimeSpan(1,2,3,4,120);
  Writeln(span.ToString('g'),' ',span.ToString('G'));
  Writeln(TimeSpan.Zero.ToString('g'),' ',TimeSpan.Zero.ToString('G'));
end.
