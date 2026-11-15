begin
  var b := new StringBuilder('Шёпот');
  b.Append(42).Append(true).Append(1.5);
  Writeln(b.ToString);
  var data := Arr('Ж','x','ю','Я');
  b.Clear;
  b.Append(data,1,2).Append(data);
  Writeln(b.ToString);
  b.Replace('Ж','ж').Replace('ю','Ю', 0, 2);
  Writeln(b.ToString);
  var target := Arr('*','*','*','*','*','*','*');
  b.CopyTo(1,target,2,3);
  for var i:=0 to target.Length-1 do Write(target[i]);
  Writeln;
  var none: array of char := nil;
  b.Append(none).Append(none,0,0);
  Writeln(b.Length);
  b.Clear;
  b.Append(1.2345678901234567).Append('|').Append(single(1.23456785)).Append('|');
  b.Append(1e15).Append('|').Append(1e-5).Append('|').Append(1e-4).Append('|');
  b.Append(int64(-9223372036854775807)).Append('|').Append(uint64(18446744073709551615));
  Writeln(b.ToString);
  var same := new StringBuilder('аб');
  var noBuilder: StringBuilder;
  same.Append(same).Append(noBuilder);
  Writeln(same.ToString);
end.
