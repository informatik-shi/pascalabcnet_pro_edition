uses School;
begin
  Writeln(Bin(-10),' ',Oct(511),' ',Hex(255));
  Writeln(School.Dec('ff',16),' ',Dec('1010',2),' ',ToBase(35,36));
  Writeln(BinFormat(258),'|',BinFormat(-5,2,':'));
  Writeln((255).ToBase(16),' ',(81).PerfectSquare);
  Writeln(PerfectSquare(9223372030926249001),' ',PerfectSquare(MaxInt64));
  Writeln(Round(SinDegrees(30),3),' ',Round(CosDegrees(60),3),' ',Round(TanDegrees(45),3));
  var n:=7; Dec(n); Writeln(n);
end.
