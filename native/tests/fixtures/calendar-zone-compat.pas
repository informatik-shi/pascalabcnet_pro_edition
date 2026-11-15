uses System;
begin
  var u:=new DateTime(2024,2,29,13,5,9,120,DateTimeKind.Utc);
  u:=u.AddTicks(3456);
  var local:=u.ToLocalTime;
  Writeln(local.Kind,' ',local.ToUniversalTime=u,' ',local.ToLocalTime=local);
  var assumed:=DateTime.SpecifyKind(u,DateTimeKind.Unspecified);
  Writeln(assumed.ToLocalTime=local,' ',DateTime.SpecifyKind(local,DateTimeKind.Unspecified).ToUniversalTime=u);
  Writeln(u.ToUniversalTime.Kind,' ',u.IsDaylightSavingTime);
  var offset:=DateTime.Parse('2024-02-29T15:35:09.1203456+02:30');
  Writeln(offset.Kind,' ',offset.ToUniversalTime.ToString('O'));
  var compact:=DateTime.Parse('2024-02-29T07:35:09.1203456-0530');
  Writeln(compact.Kind,' ',compact.ToUniversalTime=u);
  var explicitUtc:=DateTime.Parse('2024-02-29T13:05:09.1203456Z');
  Writeln(explicitUtc.Kind,' ',explicitUtc=local);
  var ok:=DateTime.TryParse('2024-02-29T13:05:09+14:99',offset);
  Writeln(ok,' ',offset=DateTime.MinValue);
  Writeln(local.ToString('U')=u.ToString('F'));
end.
