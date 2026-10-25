uses Arrays;
begin
  var m:=new integer[2,2];
  m[0,0]:=1; m[0,1]:=2; m[1,0]:=3; m[1,1]:=4;
  WriteMatrix(m,2);
  var old:=m;
  SetLength(m,1,3);
  Writeln(m.RowCount,' ',m.ColCount,' ',m[0,0],m[0,1],m[0,2],' ',old[1,1]);
  SetRandomSeed(1);
  var r:=Arrays.CreateRandomIntegerMatrix(1,3);
  r.Write('|');
  var v:=new real[1,2]; v[0,0]:=1.5; v[0,1]:=2.25;
  WriteMatrix(v,0,-2);
  Writeln('[',FormatValue('Я',3),']');
end.
