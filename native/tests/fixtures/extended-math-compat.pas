begin
  Writeln(GCD(-84,30),' ',GCD(0,0),' ',GCD(MaxInt64,0));
  Writeln(LCM(-12,18),' ',LCM(0,-9223372036854775808));
  Writeln(LCM(3037000499,3037000500));
  Writeln(Hypot(3,4),' ',Hypot(-5,-12),' ',Hypot(0,0));
  Writeln(Hypot(1e300,1e300)>1e300,' ',Hypot(1e-300,0)>0);
  Writeln(Lerp(10,20,0.25),' ',Remap(5,0,10,100,200));
  Writeln(Abs(Atan2(1,0)-Pi/2)<1e-15);
end.
