uses System;
begin
  var d := new DateTime(2024,2,29,13,5,9,123);
  Writeln(d.Year,' ',d.Month,' ',d.Day,' ',d.Hour,' ',d.Minute,' ',d.Second,' ',d.Millisecond);
  Writeln(d.DayOfWeek,' ',d.DayOfYear,' ',d.Kind,' ',d.Ticks);
  Writeln(d.GetType.FullName,' ',d.TimeOfDay.GetType.FullName);
  Writeln(d.ToString,' ',d.ToString('O'));
  Writeln(d.Date.ToString('O'),' ',d.AddMonths(12).ToString('O'),' ',d.AddYears(-1).ToString('O'));
  Writeln(d.AddDays(1.5).ToString('O'),' ',d.AddTicks(1).ToString('O'));
  var delta := new TimeSpan(1,2,3,4,5);
  Writeln(delta,' ',delta.Days,' ',delta.Hours,' ',delta.Minutes,' ',delta.Seconds,' ',delta.Milliseconds);
  Writeln(delta.Ticks,' ',TimeSpan.FromSeconds(1.2345).Ticks,' ',TimeSpan.FromSeconds(-1.2345).Ticks);
  Writeln((d+delta-d).Ticks,' ',(d-delta<d),' ',(d.Add(delta)=d+delta),' ',d.CompareTo(d.Date));
  var negative := delta.Negate;
  Writeln(negative,' ',negative.Days,' ',negative.Hours,' ',negative.Minutes,' ',negative.Seconds,' ',negative.Milliseconds);
  Writeln(negative.Duration=delta,' ',delta.Add(negative).Ticks);
  Writeln(DateTime.IsLeapYear(1900),' ',DateTime.IsLeapYear(2000),' ',DateTime.DaysInMonth(2024,2));
  Writeln(DateTime.MinValue.ToString('O'),' ',DateTime.MaxValue.ToString('O'));
  var stamped := DateTime.SpecifyKind(d,DateTimeKind.Utc);
  Writeln(stamped.ToString('O'),' ',stamped.Kind,' ',stamped=d,' ',stamped.Equals(d));
  Writeln(d.ToShortDateString,' ',d.ToShortTimeString);
end.
