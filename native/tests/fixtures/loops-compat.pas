var n,calls:integer;
function Done:boolean;
begin Inc(calls); Result:=n=3; end;
function Start:integer;
begin Write('S'); Result:=1; end;
function Finish:integer;
begin Write('F'); Result:=2; end;
function Recur(n:integer):integer;
begin
  Result:=1;
  for var i:=1 to n do Result:=Result+Recur(i-1);
end;
begin
  var sum:=0;
  for var i:=1 to 10 do sum:=sum+i;
  Writeln(sum);
  for var i:=3 downto 1 do Write(i);
  Writeln;
  var counter:=0;
  for counter:=5 to 2 do Write('bad');
  Writeln(counter);
  for var i:=MaxInt to MaxInt do begin Writeln(i); continue; end;
  for var i:=-9223372036854775808 downto -9223372036854775808 do Writeln(i);
  for var i:int64:=MaxInt64-1 to MaxInt64 do Writeln(i);
  for var i:=Start to Finish do Write(i);
  Write('|'); for counter:=Start to Finish do Write(counter); Writeln;
  repeat Inc(n); continue; until Done;
  Writeln(n,' ',calls);
  var count:=0;
  for var i:=1 to 3 do
    for var j:=1 to 5 do
    begin
      if j=3 then break;
      Inc(count);
    end;
  Writeln(count);
  repeat var x:=count; Dec(count); until x=3;
  Writeln(count,' ',Recur(4));
end.
