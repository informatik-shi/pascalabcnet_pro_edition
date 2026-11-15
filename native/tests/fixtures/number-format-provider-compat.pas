begin
  var f := NumberFormat('::','_');
  f.NumberGroupSizes := Arr(2,3,0);
  f.NumberNegativePattern := 0;
  f.NumberDecimalDigits := 3;
  f.PositiveSign := 'plus';
  f.NegativeSign := 'minus';
  var b := new StringBuilder;
  b.AppendFormat(f, '{0:N}|{1:G}|{2:E1}|{3:D4}|{4:F}',
      -123456789.5, -12.5, 125000.0, -7, -2.5);
  Writeln(b.ToString);
  var args := new object[2];
  args[0] := -12.5;
  args[1] := 12345;
  f.NumberNegativePattern:=4;
  f.NumberGroupSizes:=Arr(3);
  b.Clear;
  b.AppendFormat(f, '{0:N1} {1:N0}', args);
  Writeln(b.ToString);
  var current := NumberFormatInfo.CurrentInfo;
  current.NumberGroupSeparator:='~';
  current.NumberDecimalSeparator:='^';
  var absent: NumberFormatInfo;
  b.Clear;
  b.AppendFormat(absent, '{0:N2}', 1234.5);
  Writeln(b.ToString);
  b.Clear;
  b.AppendFormat('{0:N2}', 1234.5);
  Writeln(b.ToString);
  Writeln(Format('{0:N2}',1234.5));
  b.Clear;
  b.Append(2.5).Insert(0,1.25);
  Writeln(b.ToString);
end.
