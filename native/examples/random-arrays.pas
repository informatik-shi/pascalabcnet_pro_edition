uses Arrays;
begin
  Randomize(1);
  var a:=CreateRandomIntegerArray(3);
  Write('Arrays, 0..99: ');
  foreach var value in a do Write(value,'|');
  Writeln;
  SetRandomSeed(1);
  var b:=ArrRandomInteger(3);
  Write('PABCSystem, 0..100: ');
  foreach var value in b do Write(value,'|');
  Writeln;
  Randomize(1);
  a.FillRandom(10,12);
  Write('После заполнения: ');
  foreach var value in a do Write(value,'|');
  Writeln;
  Randomize(1);
  Writeln('Вещественное: ',RandomReal(0,1));
end.
