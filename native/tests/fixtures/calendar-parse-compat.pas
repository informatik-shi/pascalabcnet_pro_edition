uses System;
begin
  var d := DateTime.Parse(' 3/27/2008 9:58:17 AM ');
  Writeln(d.ToString('O'));
  var ok := DateTime.TryParse('2024-02-29T13:05:09.1234567', d);
  Writeln(ok,' ',d.ToString('O'),' ',d.Kind);
  ok := DateTime.TryParse('12/31/1999 12:01 AM',d);
  Writeln(ok,' ',d.ToString('O'));
  ok := DateTime.TryParse('2/29/2023',d);
  Writeln(ok,' ',d=DateTime.MinValue);
  ok := DateTime.TryParse(nil,d);
  Writeln(ok,' ',d.Ticks);
  Writeln(DateTime.Parse('2000-01-01').ToString('O'));
  var interval := TimeSpan.Parse(' -1.02:03:04.005 ');
  Writeln(interval.Ticks,' ',interval.ToString);
  Writeln(TimeSpan.Parse('12:34').ToString,' ',TimeSpan.Parse('5').ToString);
  Writeln(TimeSpan.Parse('10675199.02:48:05.4775807')=TimeSpan.MaxValue);
  Writeln(TimeSpan.Parse('-10675199.02:48:05.4775808')=TimeSpan.MinValue);
  ok := TimeSpan.TryParse('0.12:34:56.1234567',interval);
  Writeln(ok,' ',interval.Ticks);
  ok := TimeSpan.TryParse('1.99:10:00',interval);
  Writeln(ok,' ',interval.Ticks);
  ok := TimeSpan.TryParse('10675199.02:48:05.4775808',interval);
  Writeln(ok,' ',interval=TimeSpan.Zero);
  var slots := new DateTime[2];
  ok := DateTime.TryParse('2/29/24',slots[1]);
  Writeln(ok,' ',slots[1].ToString('s'),' ',slots[0].Ticks);
  Writeln(TimeSpan.Parse('1:2:3:4').Ticks,' ',TimeSpan.TryParse('+1',interval),' ',interval.Ticks);
end.
