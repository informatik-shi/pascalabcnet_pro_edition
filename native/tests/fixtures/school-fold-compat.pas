uses School;
begin
  var numbers := Arr(int64(18),30,42);
  Writeln(GCD(numbers),' ',LCM(numbers),' ',GCD(18,30,42));
  Writeln(School.GCD(18,30),' ',School.LCM(4,6));
  Writeln(GCD(-7),' ',LCM(-7),' ',GCD(-9223372036854775808));
  Writeln(School.LCM(-4,6),' ',School.LCM(4,-6),' ',School.LCM(-4,-6));
  Writeln(GCD(0,0),' ',LCM(0,0),' ',LCM(MaxInt64,MaxInt64));
end.
