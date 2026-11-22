program BoundMethodRepeatOracle;
type Probe=class
public
  function Value:integer:=7;
  function VirtualValue:integer;virtual:=8;
end;
begin
  var instance:=new Probe;
  var first,second:()->integer;
  var virtualFirst,virtualSecond:()->integer;
  for var i:=0 to 1 do
  begin
    var current:()->integer:=instance.Value;
    var virtualCurrent:()->integer:=instance.VirtualValue;
    if i=0 then
    begin
      first:=current;
      virtualFirst:=virtualCurrent;
    end
    else
    begin
      second:=current;
      virtualSecond:=virtualCurrent;
    end;
  end;
  Writeln(first=second,'|',virtualFirst=virtualSecond);
end.
